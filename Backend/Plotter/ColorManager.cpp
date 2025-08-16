#include "ColorManager.h"

namespace shmea {

ColorManager::ColorManager() {
    // Initialize the colors
    initialize();
}

ColorManager::~ColorManager() {
    // Nothing to clean up
}

void ColorManager::initialize() {
    // Initialize the color palette based on exact values from the CSS file
    
    // Background colors from CSS dark theme
    elementColors["bgGradientTop"] = RGBA(0x02, 0x13, 0x31, 0xFF);    // #021331 from histogram CSS
    elementColors["bgGradientBottom"] = RGBA(0x00, 0x0B, 0x1E, 0xFF); // #000B1E darker tone for gradient
    
    // Grid and axes colors from CSS 
    elementColors["majorGrid"] = RGBA(0x19, 0x23, 0x35, 0x80);        // --chart-lines with transparency
    elementColors["minorGrid"] = RGBA(0x19, 0x23, 0x35, 0x40);        // Lighter grid lines
    elementColors["axes"] = RGBA(0xFF, 0xFF, 0xFF, 0xCC);             // White axes with slight transparency
    elementColors["border"] = RGBA(0xCD, 0xD5, 0xE5, 0x26);           // Border with transparency
    
    // Text colors from CSS
    elementColors["title"] = RGBA(0xFF, 0xFF, 0xFF, 0xFF);            // Pure white titles
    elementColors["axisLabel"] = RGBA(0xCD, 0xD5, 0xE5, 0xCC);        // Light gray for labels
    elementColors["legend"] = RGBA(0xFF, 0xFF, 0xFF, 0xEE);           // Nearly white for legend text
    
    // Info box colors from CSS
    elementColors["legendBgTop"] = RGBA(0x23, 0x0B, 0x6A, 0xF0);      // Purple gradient top
    elementColors["legendBgBottom"] = RGBA(0x15, 0x21, 0x56, 0xF0);   // Blue gradient bottom
    
    // Theme colors from CSS semantic colors - EXACT matches from histogram_with_labels.css
    themeColors.clear(); // Clear any existing colors
    themeColors.push_back(RGBA(0x5C, 0xE9, 0xFF, 0xFF));              // #5CE9FF - Bright cyan
    themeColors.push_back(RGBA(0x5C, 0xFF, 0xB3, 0xFF));              // #5CFFB3 - Bright aquamarine
    themeColors.push_back(RGBA(0xBA, 0xB1, 0xFF, 0xFF));              // #BAB1FF - Light purple
    themeColors.push_back(RGBA(0xD6, 0xFF, 0xC7, 0xFF));              // #D6FFC7 - Light green
    themeColors.push_back(RGBA(0x51, 0xE3, 0xAD, 0xFF));              // #51E3AD - Medium aquamarine
    themeColors.push_back(RGBA(0x42, 0xCD, 0xFF, 0xFF));              // #42CDFF - Medium blue
    themeColors.push_back(RGBA(0x8C, 0xFF, 0xF9, 0xFF));              // #8CFFF9 - Light cyan
    themeColors.push_back(RGBA(0x8B, 0xF4, 0xB8, 0xFF));              // #8BF4B8 - Light green
    
    // Special chart colors with more vibrance
    elementColors["bullish"] = RGBA(0x33, 0xF5, 0x9B, 0xFF);          // Bright green for bullish candles
    elementColors["bearish"] = RGBA(0xFF, 0x5C, 0x74, 0xFF);          // Bright pink/red for bearish candles
    
    // Dark versions of colors for contrast and accents
    elementColors["cluster1Dark"] = RGBA(0x0A, 0x71, 0x43, 0xFF);     // Dark green
    elementColors["cluster2Dark"] = RGBA(0x11, 0x5E, 0x79, 0xFF);     // Dark cyan
    elementColors["cluster3Dark"] = RGBA(0x47, 0x18, 0xBF, 0xFF);     // Deep purple
    elementColors["cluster4Dark"] = RGBA(0x17, 0x69, 0x0B, 0xFF);     // Dark green
    
    // Additional accent colors
    elementColors["highlight"] = RGBA(0xFF, 0xFF, 0xFF, 0x80);        // White highlight 50% opacity
    elementColors["shadow"] = RGBA(0x00, 0x00, 0x00, 0x80);           // Black shadow 50% opacity
    elementColors["innerShadowBg"] = RGBA(0x19, 0x23, 0x35, 0x03);    // Very slight inner shadow
    
    // Colors for cluster shadows from CSS files
    elementColors["cluster1Shadow"] = RGBA(0x5C, 0xFF, 0xB3, 0x80);   // Aquamarine shadow
    elementColors["cluster2Shadow"] = RGBA(0x5C, 0xE9, 0xFF, 0x80);   // Cyan shadow
    elementColors["cluster3Shadow"] = RGBA(0xBA, 0xB1, 0xFF, 0x80);   // Purple shadow
    elementColors["cluster4Shadow"] = RGBA(0xD6, 0xFF, 0xC7, 0x80);   // Green shadow
}

void ColorManager::initialize10ClusterScheme() {
    // Clear any existing theme colors
    themeColors.clear();
    
    // Create the 10-cluster color scheme using exact colors from the CSS semantic colors
    
    // Fill colors - using exactly the CSS semantic values for --charts-group-X-fill
    themeColors.push_back(RGBA(0x5C, 0xFF, 0xB3, 0xFF)); // --charts-group-1-fill: var(--aquamarine-300) #5CFFB3
    themeColors.push_back(RGBA(0x5C, 0xE9, 0xFF, 0xFF)); // --charts-group-2-fill: var(--spray-300) #5CE9FF
    themeColors.push_back(RGBA(0xBA, 0xB1, 0xFF, 0xFF)); // --charts-group-3-fill: var(--blue-marguerite-300) #BAB1FF
    themeColors.push_back(RGBA(0xD6, 0xFF, 0xC7, 0xFF)); // --charts-group-4-fill: var(--screamin-green-100) #D6FFC7
    themeColors.push_back(RGBA(0x5C, 0x9D, 0xFF, 0xFF)); // --charts-group-5-fill: var(--cornflower-blue-400) #5C9DFF
    themeColors.push_back(RGBA(0x01, 0xB8, 0x63, 0xFF)); // --charts-group-6-fill: var(--aquamarine-600) #01B863
    themeColors.push_back(RGBA(0x8C, 0x4A, 0x72, 0xFF)); // --charts-group-7-fill: var(--wine-berry-700) #8C4A72
    themeColors.push_back(RGBA(0x01, 0x92, 0xB9, 0xFF)); // --charts-group-8-fill: var(--spray-600) #0192B9
    themeColors.push_back(RGBA(0x1A, 0xB1, 0x00, 0xFF)); // --charts-group-9-fill: var(--screamin-green-600) #1AB100
    themeColors.push_back(RGBA(0x1F, 0x57, 0xF1, 0xFF)); // --charts-group-10-fill: var(--cornflower-blue-600) #1F57F1
    
    // Set shadow colors with 50% alpha for effects like shadows
    elementColors["cluster1Shadow"] = RGBA(0x5C, 0xFF, 0xB3, 0x80); // Group 1 with 50% alpha
    elementColors["cluster2Shadow"] = RGBA(0x5C, 0xE9, 0xFF, 0x80); // Group 2 with 50% alpha
    elementColors["cluster3Shadow"] = RGBA(0xBA, 0xB1, 0xFF, 0x80); // Group 3 with 50% alpha
    elementColors["cluster4Shadow"] = RGBA(0xD6, 0xFF, 0xC7, 0x80); // Group 4 with 50% alpha
    elementColors["cluster5Shadow"] = RGBA(0x5C, 0x9D, 0xFF, 0x80); // Group 5 with 50% alpha
    elementColors["cluster6Shadow"] = RGBA(0x01, 0xB8, 0x63, 0x80); // Group 6 with 50% alpha
    elementColors["cluster7Shadow"] = RGBA(0x8C, 0x4A, 0x72, 0x80); // Group 7 with 50% alpha
    elementColors["cluster8Shadow"] = RGBA(0x01, 0x92, 0xB9, 0x80); // Group 8 with 50% alpha
    elementColors["cluster9Shadow"] = RGBA(0x1A, 0xB1, 0x00, 0x80); // Group 9 with 50% alpha
    elementColors["cluster10Shadow"] = RGBA(0x1F, 0x57, 0xF1, 0x80); // Group 10 with 50% alpha
    
    // Set stroke colors - using exactly the CSS semantic values for --charts-group-X-stroke
    elementColors["cluster1Stroke"] = RGBA(0x5C, 0xFF, 0xB3, 0xFF); // --charts-group-1-stroke: var(--aquamarine-300)
    elementColors["cluster2Stroke"] = RGBA(0x5C, 0xE9, 0xFF, 0xFF); // --charts-group-2-stroke: var(--spray-300)
    elementColors["cluster3Stroke"] = RGBA(0xBA, 0xB1, 0xFF, 0xFF); // --charts-group-3-stroke: var(--blue-marguerite-300)
    elementColors["cluster4Stroke"] = RGBA(0xD6, 0xFF, 0xC7, 0xFF); // --charts-group-4-stroke: var(--screamin-green-100)
    elementColors["cluster5Stroke"] = RGBA(0x5C, 0x9D, 0xFF, 0xFF); // --charts-group-5-stroke: var(--cornflower-blue-400)
    elementColors["cluster6Stroke"] = RGBA(0x33, 0xF5, 0x9B, 0xFF); // --charts-group-6-stroke: var(--aquamarine-400)
    elementColors["cluster7Stroke"] = RGBA(0xCD, 0x99, 0xBE, 0xFF); // --charts-group-7-stroke: var(--wine-berry-400)
    elementColors["cluster8Stroke"] = RGBA(0x1A, 0xD5, 0xF6, 0xFF); // --charts-group-8-stroke: var(--spray-400)
    elementColors["cluster9Stroke"] = RGBA(0x4C, 0xF6, 0x25, 0xFF); // --charts-group-9-stroke: var(--screamin-green-400)
    elementColors["cluster10Stroke"] = RGBA(0x5C, 0x9D, 0xFF, 0xFF); // --charts-group-10-stroke: var(--cornflower-blue-400)
    
    // Set dark colors - using exactly the CSS semantic values for --charts-group-X-dark
    elementColors["cluster1Dark"] = RGBA(0x0A, 0x71, 0x43, 0xFF); // --charts-group-1-dark: var(--aquamarine-800)
    elementColors["cluster2Dark"] = RGBA(0x11, 0x5E, 0x79, 0xFF); // --charts-group-2-dark: var(--spray-800)
    elementColors["cluster3Dark"] = RGBA(0x47, 0x18, 0xBF, 0xFF); // --charts-group-3-dark: var(--blue-marguerite-800)
    elementColors["cluster4Dark"] = RGBA(0x17, 0x69, 0x0B, 0xFF); // --charts-group-4-dark: var(--screamin-green-800)
    elementColors["cluster5Dark"] = RGBA(0x19, 0x36, 0xB4, 0xFF); // --charts-group-5-dark: var(--cornflower-blue-800)
    elementColors["cluster6Dark"] = RGBA(0x00, 0x34, 0x1E, 0xFF); // --charts-group-6-dark: var(--aquamarine-950)
    elementColors["cluster7Dark"] = RGBA(0x44, 0x22, 0x36, 0xFF); // --charts-group-7-dark: var(--wine-berry-950)
    elementColors["cluster8Dark"] = RGBA(0x06, 0x33, 0x46, 0xFF); // --charts-group-8-dark: var(--spray-950)
    elementColors["cluster9Dark"] = RGBA(0x05, 0x32, 0x01, 0xFF); // --charts-group-9-dark: var(--screamin-green-950)
    elementColors["cluster10Dark"] = RGBA(0x15, 0x21, 0x56, 0xFF); // --charts-group-10-dark: var(--cornflower-blue-950)
}

RGBA ColorManager::getThemeColor(int index) const {
    if (themeColors.empty()) {
        // Return default color if theme colors are empty
        return RGBA(0xFF, 0xFF, 0xFF, 0xFF);
    }
    
    // Use modulo to wrap around if index is out of bounds
    return themeColors[index % themeColors.size()];
}

RGBA ColorManager::getElementColor(const std::string& element) const {
    std::map<std::string, RGBA>::const_iterator it = elementColors.find(element);
    if (it != elementColors.end()) {
        return it->second;
    }
    
    // Return white as default if element not found
    return RGBA(0xFF, 0xFF, 0xFF, 0xFF);
}

bool ColorManager::hasElementColor(const std::string& element) const {
    return elementColors.find(element) != elementColors.end();
}

RGBA ColorManager::blendColors(const RGBA& baseColor, const RGBA& overlayColor, float alpha) const {
    return RGBA(
        static_cast<unsigned char>(baseColor.r * (1.0f - alpha) + overlayColor.r * alpha),
        static_cast<unsigned char>(baseColor.g * (1.0f - alpha) + overlayColor.g * alpha),
        static_cast<unsigned char>(baseColor.b * (1.0f - alpha) + overlayColor.b * alpha),
        baseColor.a  // Keep the original alpha
    );
}

RGBA ColorManager::blendRGBA(const RGBA& base, const RGBA& over) const {
    // If the overlay is fully transparent, return the base unchanged
    if (over.a == 0) return base;
    
    // If the overlay is fully opaque, return it directly
    if (over.a == 255) return over;
    
    // Calculate alpha values for blending
    float alphaOver = over.a / 255.0f;
    float alphaBase = base.a / 255.0f;
    float alphaOut = alphaOver + alphaBase * (1.0f - alphaOver);
    
    // If the resulting alpha is zero, return transparent black
    if (alphaOut < 0.001f) return RGBA(0, 0, 0, 0);
    
    // Blend the colors properly considering the alpha channels
    unsigned char r = static_cast<unsigned char>((over.r * alphaOver + base.r * alphaBase * (1.0f - alphaOver)) / alphaOut);
    unsigned char g = static_cast<unsigned char>((over.g * alphaOver + base.g * alphaBase * (1.0f - alphaOver)) / alphaOut);
    unsigned char b = static_cast<unsigned char>((over.b * alphaOver + base.b * alphaBase * (1.0f - alphaOver)) / alphaOut);
    unsigned char a = static_cast<unsigned char>(alphaOut * 255.0f);
    
    return RGBA(r, g, b, a);
}

RGBA ColorManager::HSVtoRGBA(float h, float s, float v)
{
    float r, g, b;
    int i = static_cast<int>(h * 6.0f);

    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    switch (i % 6)
    {
	case 0: r = v; g = t; b = p; break;
	case 1: r = q; g = v; b = p; break;
	case 2: r = p; g = v; b = t; break;
	case 3: r = p; g = q; b = v; break;
	case 4: r = t; g = p; b = v; break;
	case 5: r = v; g = p; b = q; break;
	default: r = g = b = 0; break;

    }

    return RGBA(0x00,
		static_cast<unsigned char>(r * 255),
		static_cast<unsigned char>(g * 255),
		static_cast<unsigned char>(b * 255));
}
const std::vector<RGBA>& ColorManager::getThemeColors() const {
    return themeColors;
}

const std::map<std::string, RGBA>& ColorManager::getElementColors() const {
    return elementColors;
}

void ColorManager::setThemeColors(const std::vector<RGBA>& colors) {
    themeColors = colors;
}

} // namespace shmea 
