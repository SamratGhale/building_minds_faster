#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <iomanip>
namespace fs = std::filesystem;

int main(int argc, char* argv[]){

  std::string path = ".";
  if(argc > 1){
    path = argv[1];
  }
  
  int total_lines = 0;
  int total_spaces = 0;
  for(const auto & entry: fs::directory_iterator(path)){
    std::fstream newFile;
    newFile.open(entry.path(), std::ios::in);
    if(newFile.is_open()){
      std::string tp;
      int count = 0;
      int spaces = 0;
      if(entry.path().extension() == ".cpp" ||entry.path().extension() == ".h" ||  entry.path().extension() == ".hpp" || entry.path().extension() == ".c" ){
        while(getline(newFile, tp)){
          //std::cout << tp << "";
          if(tp.find_first_not_of(' ') == std::string::npos){
            spaces++;
          }
          count++;
        }
        std::cout << std::setw(25) << entry.path().filename() << std::setw(5) << " : "<<  std::setw(5) << count << std::setw(5) <<  std::endl;
        total_lines += count;
        total_spaces += spaces;
      }
      newFile.close();
    }
  }
  std::cout << std::endl;
 std::cout << "total lines: "<< total_lines << std::endl;
  std::cout << "total spaces: "<< total_spaces<< std::endl;
}