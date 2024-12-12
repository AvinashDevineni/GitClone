#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <unordered_set>
#include <functional>

const char* name = "gitclone";
const char* version = "0.0.1";

const char* ignoreFileName = ".ignore.txt";

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

std::unordered_set<std::string> tryReadIgnoreFile(const std::string& CWD) {
    std::ifstream ignoreFile(CWD + "\\" + ignoreFileName);
    if (!ignoreFile)
        return {};

    std::unordered_set<std::string> ignorePaths;
    std::string line;
    while (std::getline(ignoreFile, line))
        ignorePaths.insert(line);

    ignoreFile.close();
    return ignorePaths;
}

void addFilePathsInDirToFolder(const std::string* dirPath, std::ofstream& outFile,
                               std::function<bool(std::string&)> shouldIgnoreFile) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(*dirPath)) {
        if (entry.is_directory())
            continue;
        
        std::string path = entry.path().string();
        if (shouldIgnoreFile(path))
            continue;

        outFile << path << "\n";
    }
}

bool isCwdInitialized(const std::string& CWD) {
    struct stat _;
    const std::string DATA_PATH = CWD + "\\." + name;
    return std::filesystem::exists(DATA_PATH.c_str());
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
            return 1;
        }

        const std::string ADD_PATH = DATA_PATH + "\\addpaths.txt";
        const bool SHOULD_ADD_ALL = argc == 2 || strcmp(argv[2], "all") == 0;
        std::ofstream addFile;

        // initializing add file to write or append
        // depending on if we are adding everything
        // or if the file already exists or doesn't exist
        struct stat _;
        if (SHOULD_ADD_ALL || !std::filesystem::exists(ADD_PATH.c_str()))
            addFile.open(ADD_PATH);
        else addFile.open(ADD_PATH, std::ios_base::app);

        const auto ignoredPaths = tryReadIgnoreFile(CWD);

        // adding all files to add file
        if (SHOULD_ADD_ALL) {
            addFilePathsInDirToFolder(&CWD, addFile, [DATA_PATH, ignoredPaths, CWD](std::string& PATH) -> bool{
                // PATH starts with DATA_PATH
                if (PATH.find(DATA_PATH) == 0)
                    return true;

                bool isIgnored = false;
                for (const auto& ignoredPath : ignoredPaths) {
                    if (PATH.find(CWD + "\\" + ignoredPath) == 0) {
                        isIgnored = true;
                        return true;
                    }
                }

                return false;
            });
        }

        else {
            for (int i = 0; argv[2][i] != '\0'; i++) {
                if (argv[2][i] == '/')
                    argv[2][i] = '\\';
            }

            bool isIgnored = false;
            for (const auto& ignoredPath : ignoredPaths) {
                if (std::string(argv[2]).find(ignoredPath) == 0) {
                    isIgnored = true;
                    break;
                }
            }

            if (isIgnored) {
                std::cerr << "The file you are trying to add is in the ignore file, so it was not added.";
                return 1;
            }

            const std::string ignorePath = CWD + "\\" + argv[2];
            if (std::filesystem::is_directory(ignorePath)) {
                addFilePathsInDirToFolder(&ignorePath, addFile, [DATA_PATH, ignoredPaths, CWD](std::string& PATH) -> bool{
                    // PATH starts with DATA_PATH
                    if (PATH.find(DATA_PATH) == 0)
                        return true;

                    bool isIgnored = false;
                    for (const auto& ignoredPath : ignoredPaths) {
                        if (PATH.find(CWD + "\\" + ignoredPath) == 0) {
                            isIgnored = true;
                            return true;
                        }
                    }

                    return false;
                });
            }

            else addFile << CWD + "\\" + argv[2] << "\n";
        }
    }

    else if (strcmp("ignore", CMD) == 0) {
        if (argc == 2) {
            std::cerr << "File/folder to ignore must be given.";
            return 1;
        }

        for (int i = 0; argv[2][i] != '\0'; i++) {
            if (argv[2][i] == '/')
                argv[2][i] = '\\';
        }

        const std::string IGNORE_PATH = CWD + "\\" + ignoreFileName;
        std::ofstream ignoreFile;

        struct stat _;
        if (!std::filesystem::exists(IGNORE_PATH.c_str()))
            ignoreFile.open(IGNORE_PATH);
        else ignoreFile.open(IGNORE_PATH, std::ios_base::app);

        ignoreFile << argv[2] << "\n";
        ignoreFile.close();
    }
}
