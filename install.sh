
{
  cmake ../ 
} || {
  echo "$(cat /ShmeaDB/build/CMakeFiles/CMakeOutput.log)"
  echo "$(cat /ShmeaDB/build/CMakeFiles/CMakeError.log)"
  exit 1
}
