#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <unordered_set>
#include <functional>
#include <chrono>

constexpr char* name = "gitclone";
constexpr char* version = "0.0.1";

std::unordered_set<std::string> tryReadFileUniqueNoOrder(const std::string& PATH) {
    std::ifstream file(PATH);
    if (!file)
        return {};

    std::unordered_set<std::string> ignorePaths;
    std::string line;
    while (std::getline(file, line))
        ignorePaths.insert(line);

    file.close();
    return ignorePaths;
}

std::vector<std::string> tryReadFile(const std::string& PATH) {
    std::ifstream file(PATH);
    if (!file)
        return {};

    std::vector<std::string> ignorePaths;
    std::string line;
    while (std::getline(file, line))
        ignorePaths.push_back(line);

    file.close();
    return ignorePaths;
}

void addFilePathsInDirToFolder(const std::string* dirPath, std::ofstream& outFile,
                               std::function<bool(std::string&)> shouldIgnoreFile,
                               std::function<std::string(std::filesystem::path&)> getPathStr) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(*dirPath)) {
        if (entry.is_directory())
            continue;
        
        auto path = entry.path();
        std::string pathStr = getPathStr(path);
        if (shouldIgnoreFile(pathStr))
            continue;

        outFile << pathStr << "\n";
    }
}

template <typename container>
void copyFilesRelatively(container& sourcePaths, const std::string& parentPath, const std::string& targetPath) {
    for (const std::string& path : sourcePaths) {
        const std::string relPath = std::filesystem::path(path).lexically_relative(parentPath).string();
        const std::string sourcePath = parentPath + "\\" + relPath;
        const std::string finalPath = targetPath + "\\" + relPath;

        if (std::filesystem::is_directory(sourcePath)) {
            std::filesystem::create_directories(finalPath);
            for (const auto& path : std::filesystem::recursive_directory_iterator(sourcePath)) {
                const auto pathStr = path.path().lexically_relative(sourcePath).string();
                std::cout << pathStr << std::endl;

                if (path.is_directory())
                    std::filesystem::create_directories(finalPath + '\\' + pathStr);

                else {
                    std::ofstream outFile(finalPath + "\\" + pathStr);
                    for (const auto& line : tryReadFile(sourcePath + '\\' + pathStr))
                        outFile << line << "\n";
                    outFile.close();
                }
            }
        }

        else {
            // if path is in a directory, create it
            const int idx = relPath.find('\\');
            const bool isInDir = idx != std::string::npos;
            if (isInDir) {
                std::cout << relPath << " is in dir" << std::endl;
                std::filesystem::create_directories(targetPath + '\\' + relPath.substr(0, idx));
            }

            std::cout << "Created dir" << std::endl;

            std::ofstream outFile(finalPath);
            for (const auto& line : tryReadFile(sourcePath))
                outFile << line << "\n";
            outFile.close();
        }
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

    const std::string CWD = std::filesystem::current_path().string();

    const std::string DATA_PATH_REL = std::string("\\.") + name;
    const std::string ADD_PATH_REL = DATA_PATH_REL + "\\addpaths.txt";
    const std::string IGNORE_PATH_REL = "\\.ignore.txt";
    const std::string SAVES_PATH_REL = DATA_PATH_REL + "\\" + "saves";

    const std::string DATA_PATH_ABS = CWD + DATA_PATH_REL;
    const std::string ADD_PATH_ABS = CWD + ADD_PATH_REL;
    const std::string IGNORE_PATH_ABS = CWD + IGNORE_PATH_REL;
    const std::string SAVES_PATH_ABS = DATA_PATH_ABS + "\\" + "saves";

    const char* CMD = argv[1];

    // commands that don't need repo to be initialized
    if (strcmp("init", CMD) == 0) {
        std::filesystem::create_directory(DATA_PATH_ABS);
        return 0;
    }

    if (!isCwdInitialized(CWD)) {
        std::cerr << "Repository is not initialized. You probably want to run \"init\"";
        return 1;
    }

    // commands that need/should have initialized repo
    if (strcmp("add", CMD) == 0) {
        const bool SHOULD_ADD_ALL = argc == 2 || strcmp(argv[2], "all") == 0;
        std::ofstream addFile;

        if (SHOULD_ADD_ALL || !std::filesystem::exists(ADD_PATH_ABS.c_str()))
            addFile.open(ADD_PATH_ABS, std::ios::out);
        else addFile.open(ADD_PATH_ABS, std::ios::app);

        std::vector<std::string> ignoredPaths;
        for (const auto& ignoredPath : tryReadFileUniqueNoOrder(IGNORE_PATH_ABS))
            ignoredPaths.push_back(CWD + '\\' + ignoredPath);

        if (SHOULD_ADD_ALL) {
            addFilePathsInDirToFolder(&CWD, addFile, [DATA_PATH_ABS, ignoredPaths, CWD](std::string& PATH) -> bool{
                // PATH starts with DATA_PATH
                if (PATH.find(DATA_PATH_ABS) == 0)
                    return true;

                for (const auto& ignoredPath : ignoredPaths) {
                    std::cout << ignoredPath << std::endl;
                    if (PATH.find(ignoredPath) == 0)
                        return true;
                }

                return false;
            }, [CWD](std::filesystem::path& PATH) -> std::string{
                return PATH.string();
            });
        }

        else {
            for (int i = 0; argv[2][i] != '\0'; i++) {
                if (argv[2][i] == '/')
                    argv[2][i] = '\\';
            }

            // "removing" first character if it is "\"
            if (argv[2][0] == '\\')
                argv[2] = argv[2] + 1;

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

            const std::string addPath = CWD + "\\" + argv[2];
            if (std::filesystem::is_directory(addPath)) {
                addFilePathsInDirToFolder(&addPath, addFile, [DATA_PATH_REL, ignoredPaths, CWD](std::string& PATH) -> bool{
                    // PATH starts with DATA_PATH
                    if (PATH.find(DATA_PATH_REL) == 0)
                        return true;

                    bool isIgnored = false;
                    for (const auto& ignoredPath : ignoredPaths) {
                        if (PATH.find(ignoredPath) == 0) {
                            isIgnored = true;
                            return true;
                        }
                    }

                    return false;
                }, [CWD](std::filesystem::path& PATH) -> std::string{
                    return PATH.string();
                });
            }

            else {
                // making path absolute in case it was entered as absolute
                if (std::filesystem::path(argv[2]).is_absolute())
                    addFile << argv[2] << "\n";
                else addFile << CWD + argv[2] << "\n";
            }
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

        std::ofstream ignoreFile;
        if (!std::filesystem::exists(IGNORE_PATH_ABS.c_str()))
            ignoreFile.open(IGNORE_PATH_ABS);
        else ignoreFile.open(IGNORE_PATH_ABS, std::ios_base::app);

        ignoreFile << argv[2] << "\n";
        ignoreFile.close();
    }

    else if (strcmp("save", CMD) == 0) {
        if (argc == 2) {
            std::cerr << "Message must be specified in quotes after \"save\" command";
            return 1;
        }

        if (!std::filesystem::exists(SAVES_PATH_ABS.c_str()))
            std::filesystem::create_directory(SAVES_PATH_ABS);

        const std::string time = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        const std::string SAVE_PATH_ABS = SAVES_PATH_ABS + "\\" + time;
        std::filesystem::create_directory(SAVE_PATH_ABS);

        std::ofstream saveMetadataFile(SAVE_PATH_ABS + "\\." + name + ".savedata.txt");
        saveMetadataFile << argv[2] << "\n" << time;

        const auto CWD_LEN = CWD.length();
        auto addedPaths = tryReadFileUniqueNoOrder(ADD_PATH_ABS);
        copyFilesRelatively(addedPaths, CWD, SAVE_PATH_ABS);
    }

    else if (strcmp("revert", CMD) == 0) {
        if (!std::filesystem::exists(SAVES_PATH_ABS)) {
            std::cerr << "No saves have been created, so there is nothing to revert to";
            return 1;
        }

        if (argc == 2) {
            long long latestTime = -1;
            for (const auto& path : std::filesystem::directory_iterator(SAVES_PATH_ABS)) {
                const auto pathStr = path.path().string();
                const auto saveFileName = pathStr.substr(pathStr.find_last_of('\\') + 1);
                
                long long saveTime = std::stoll(saveFileName);
                if (saveTime > latestTime)
                    latestTime = saveTime;
            }

            const std::string latestSavePath = SAVES_PATH_ABS + "\\" + std::to_string(latestTime);

            std::vector<std::string> pathsInSave;
            for (const auto& path : std::filesystem::recursive_directory_iterator(latestSavePath)) {
                if (path.is_directory())
                    continue;
                pathsInSave.push_back(path.path().string());
            }

            copyFilesRelatively(pathsInSave, latestSavePath, CWD);
        }
    }

    else {
        std::cerr << "Command not recognized";
        return 1;
    }
}
