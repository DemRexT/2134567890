#include "CertificateGenerator.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

void printUsage(const char *programName) {
    std::cout << "Usage: " << programName
              << " --template <template_file> --data <data_file> --styles <styles_dir>"
                 " --style <style_name> --output <output_dir>\n";
}

int main(int argc, char *argv[]) {
    std::string templatePath;
    std::string dataPath;
    std::string stylesDirectory;
    std::string styleName;
    std::string outputDirectory = "generated";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--template" && i + 1 < argc) {
            templatePath = argv[++i];
        } else if (arg == "--data" && i + 1 < argc) {
            dataPath = argv[++i];
        } else if (arg == "--styles" && i + 1 < argc) {
            stylesDirectory = argv[++i];
        } else if (arg == "--style" && i + 1 < argc) {
            styleName = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            outputDirectory = argv[++i];
        } else if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    if (templatePath.empty() || dataPath.empty() || stylesDirectory.empty() || styleName.empty()) {
        std::cerr << "Missing required arguments.\n";
        printUsage(argv[0]);
        return 1;
    }

    try {
        auto styles = loadVisualStyles(stylesDirectory);
        auto styleIt = styles.find(styleName);
        if (styleIt == styles.end()) {
            std::cerr << "Style '" << styleName << "' not found in " << stylesDirectory << "\n";
            std::cerr << "Available styles:\n";
            for (const auto &pair : styles) {
                std::cerr << "  - " << pair.first << "\n";
            }
            return 1;
        }

        CertificateGenerator generator(templatePath, dataPath, outputDirectory, styleIt->second);
        generator.run();
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
