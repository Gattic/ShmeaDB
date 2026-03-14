// Copyright 2026 Robert Carneiro, Derek Meer, Matthew Tabak, Eric Lujan
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#include "udp_channel.h"
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <cstdio>

namespace GNet {

UDPChannel::UDPChannel() : m_socket(-1) {
    m_peerMutex = new pthread_mutex_t;
    pthread_mutex_init(m_peerMutex, NULL);
}

UDPChannel::~UDPChannel() {
    Close();
    pthread_mutex_destroy(m_peerMutex);
    delete m_peerMutex;
}

bool UDPChannel::Open(const shmea::GString& port) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port.c_str(), &hints, &res) != 0) return false;

    m_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (m_socket < 0) { freeaddrinfo(res); return false; }

    int opt = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(m_socket, res->ai_addr, res->ai_addrlen) < 0) {
        close(m_socket);
        m_socket = -1;
        freeaddrinfo(res);
        return false;
    }
    freeaddrinfo(res);

    // Set non-blocking
    int flags = fcntl(m_socket, F_GETFL, 0);
    fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);

    return true;
}

void UDPChannel::Close() {
    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
    }
    pthread_mutex_lock(m_peerMutex);
    m_peers.clear();
    pthread_mutex_unlock(m_peerMutex);
}

void UDPChannel::Send(int peerIndex, const void* data, size_t len) {
    if (m_socket < 0) return;

    pthread_mutex_lock(m_peerMutex);
    if (peerIndex < 0 || peerIndex >= (int)m_peers.size() || m_peers[peerIndex].addrLen == 0) {
        pthread_mutex_unlock(m_peerMutex);
        return;
    }
    UDPPeer peer = m_peers[peerIndex];
    pthread_mutex_unlock(m_peerMutex);

    sendto(m_socket, data, len, 0, (struct sockaddr*)&peer.addr, peer.addrLen);
}

void UDPChannel::Broadcast(const void* data, size_t len) {
    if (m_socket < 0) return;

    pthread_mutex_lock(m_peerMutex);
    for (size_t i = 0; i < m_peers.size(); ++i) {
        if (m_peers[i].addrLen > 0) {
            sendto(m_socket, data, len, 0,
                   (struct sockaddr*)&m_peers[i].addr, m_peers[i].addrLen);
        }
    }
    pthread_mutex_unlock(m_peerMutex);
}

int UDPChannel::Receive(void* buffer, size_t maxLen, sockaddr_storage& outAddr) {
    if (m_socket < 0) return -1;

    socklen_t addrLen = sizeof(outAddr);
    memset(&outAddr, 0, sizeof(outAddr));

    ssize_t bytes = recvfrom(m_socket, buffer, maxLen, 0,
                             (struct sockaddr*)&outAddr, &addrLen);

    if (bytes < 0) return 0;  // EAGAIN/EWOULDBLOCK — no data
    return (int)bytes;
}

int UDPChannel::AddPeer(const sockaddr_storage& addr, socklen_t addrLen) {
    pthread_mutex_lock(m_peerMutex);

    // Reuse tombstoned slot
    for (size_t i = 0; i < m_peers.size(); ++i) {
        if (m_peers[i].addrLen == 0) {
            m_peers[i].addr = addr;
            m_peers[i].addrLen = addrLen;
            pthread_mutex_unlock(m_peerMutex);
            return (int)i;
        }
    }

    // No empty slot — grow
    UDPPeer peer;
    peer.addr = addr;
    peer.addrLen = addrLen;
    m_peers.push_back(peer);
    int idx = (int)m_peers.size() - 1;

    pthread_mutex_unlock(m_peerMutex);
    return idx;
}

void UDPChannel::RemovePeer(int peerIndex) {
    pthread_mutex_lock(m_peerMutex);
    if (peerIndex >= 0 && peerIndex < (int)m_peers.size()) {
        memset(&m_peers[peerIndex], 0, sizeof(UDPPeer));
        // addrLen = 0 marks as tombstoned
    }
    pthread_mutex_unlock(m_peerMutex);
}

} // namespace GNet
