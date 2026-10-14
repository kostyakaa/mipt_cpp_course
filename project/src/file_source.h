#ifndef NANO_EDR_FILE_SOURCE_H
#define NANO_EDR_FILE_SOURCE_H

#include <fstream>
#include <string>

namespace nano_edr {

class Agent;

class FileSource {
 public:
    explicit FileSource(const std::string& path);

    void Run(Agent* agent);

 private:
    std::ifstream input_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_FILE_SOURCE_H
