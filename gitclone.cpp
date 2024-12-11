#include <iostream>
#include <fstream>
#include <filesystem>
#include <sys/stat.h>
#include <cstring>

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

bool isCwdInitialized(const std::string& cwd) {
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

    const char* CMD = argv[1];
    const std::string CWD = std::filesystem::current_path().string();
    const std::string DATA_PATH = CWD + "\\." + name;

    if (strcmp("init", CMD) == 0)
        std::filesystem::create_directory(DATA_PATH);

    else if (strcmp("add", CMD) == 0) {
        if (!isCwdInitialized(CWD)) {
            std::cerr << "Repository is not initialized. You probably want to run \"init\"";
            return 0;
        }

        const std::string ADD_PATH = DATA_PATH + "/addpaths.txt";
        std::ofstream addFile;

        // initializing add file to write or append
        // depending on if we are adding everything
        // or if the file already exists or doesn't exist
        struct stat _;
        if ((argc == 2 || strcmp(argv[2], "all")) || stat(ADD_PATH.c_str(), &_) != 0)
            addFile.open(ADD_PATH);
        else addFile.open(ADD_PATH, std::ios_base::app);


        // adding all files to add file
        if (argc == 2 || strcmp(argv[2], "all")) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(CWD)) {
                if (entry.is_directory())
                    continue;
                
                const std::string PATH = entry.path().string();
                // PATH starts with DATA_PATH
                if (PATH.find(DATA_PATH) == 0)
                    continue;

                addFile << PATH << std::endl;
            }
        }
    }

    else if (strcmp("ignore", CMD) == 0) {
        if (argc == 2) {
            std::cerr << "File/folder to ignore must be given.";
            return 1;
        }

        if (strchr(argv[2], '/')) {
            std::cerr << "For subdirectories, use \"\\\" instead of \"/\"";
            return 1;
        }

        const std::string IGNORE_PATH = CWD + "/.ignore.txt";
        std::ofstream ignoreFile;

        struct stat _;
        int res = stat(IGNORE_PATH.c_str(), &_);
        if (res)
            std::cout << res;
        if (stat(IGNORE_PATH.c_str(), &_) != 0)
            ignoreFile.open(IGNORE_PATH);
        else ignoreFile.open(IGNORE_PATH, std::ios_base::app);

        ignoreFile << argv[2] << "\n";
        ignoreFile.close();
    }
}
