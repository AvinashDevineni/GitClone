#include <iostream>
#include <fstream>
#include <filesystem>
#include <sys/stat.h>

const std::string name = "gitclone";
const std::string version = "0.0.1";

// std::string readFile(const char* path) {
//     std::ifstream file(path);

//     if (!file) { // Check if the file was opened successfully
//         std::cerr << "Error: Could not open file " << filename << std::endl;
//         return 1;
//     }

//     std::string line;
//     while (std::getline(file, line)) { // Read the file line by line
//         std::cout << line << std::endl; // Print each line
//     }

//     file.close(); // Close the file
//     return line;
// }

bool isCwdInitialized(std::string& cwd) {
    struct stat _;
    std::string dataPath = cwd + "/." + name;
    int res = stat(dataPath.c_str(), &_);

    return res == 0;
}

int main(int argc, char ** argv) {
    if (argc == 1) {
        std::cout << name << " " << "v" << version;
        return 0;
    }

    char* cmd = argv[1];
    std::string cwd = std::filesystem::current_path().string();
    std::string dataPath = cwd + "/." + name;

    if (strcmp("init", cmd) == 0)
        std::filesystem::create_directory(dataPath);

    else if (strcmp("add", cmd) == 0) {
        if (!isCwdInitialized(cwd)) {
            std::cout << "Repository is not initialized. You probably want to run \"init\"";
            return 0;
        }

        const std::string addPath = dataPath + "/add.txt";
        std::ofstream addFile;

        // initializing add file to write or append
        // depending on if it already exists
        struct stat _;
        if (!stat(dataPath.c_str(), &_))
            addFile.open(addPath);
        else addFile.open(addPath, std::ios_base::app);


        // adding all files to add file
        if (argc == 2 || strcmp(argv[2], "all")) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(cwd)) {
                if (entry.is_directory())
                    continue;
                addFile << entry.path().string() << std::endl;
            }
        }
    }
}
