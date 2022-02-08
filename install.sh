
{
  cmake ../ 
} || {
  echo "OUTPUT!"
  echo "$(cat /ShmeaDB/build/CMakeFiles/CMakeOutput.log)"
  echo "ERROR!"
  echo "$(cat /ShmeaDB/build/CMakeFiles/CMakeError.log)"
  exit 1
}
