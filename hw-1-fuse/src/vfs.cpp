#include <iostream>
#include "fuse_driver.h"

int main(int argc, char *argv[]) {
    try {
        auto driver = fuse_driver::Driver();
        return driver.run_fuse(argc, argv);
    } catch(std::exception& e) {
        std::cerr << "Exception during work: " << e.what() << std::endl;
        return 0;
    }
}