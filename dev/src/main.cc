#include "glog/logging.h"
#include "gflags/gflags.h"
#include "experimental/cask.h"

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return experimental::cask::run();
}
