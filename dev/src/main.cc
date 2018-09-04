#include "glog/logging.h"
#include "experimental/radarconcept.h"

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  return experimental::main();
}
