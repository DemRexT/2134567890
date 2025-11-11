#include "CertificateGenerator.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace {
std::vector<std::string> splitCSVLine(const std::string &line) {
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string item;

    while (std::getline(ss, item, ';')) {
        result.push_back(item);
    }
    return result;
}

std::string sanitizeFileName(const std::string &name) {
    const std::string forbidden = "<>:\"/\\|?*";
    std::string sanitized;
    sanitized.reserve(name.size());

    for (unsigned char ch : name) {
        if (std::isspace(ch) || ch < 32 || forbidden.find(static_cast<char>(ch)) != std::string::npos) {
            sanitized.push_back('_');
        } else {
            sanitized.push_back(static_cast<char>(ch));
        }
    }

    if (sanitized.empty()) {
        sanitized = "certificate";
    }

    return sanitized;
}
}

CertificateGenerator::CertificateGenerator(std::string templatePath,
                                           std::string dataPath,
                                           std::string outputDirectory,
                                           VisualStyle style)
    : templatePath_(std::move(templatePath)),
      dataPath_(std::move(dataPath)),
      outputDirectory_(std::move(outputDirectory)),
      style_(std::move(style)) {}

void CertificateGenerator::run() {
    ensureOutputDirectory();
    loadTemplate();
    loadRecipients();

    for (const auto &recipient : recipients_) {
        auto content = fillTemplate(recipient);
        saveCertificate(recipient, content);
    }

    saveMetadataReport();
}

void CertificateGenerator::loadTemplate() {
    std::ifstream input(templatePath_);
    if (!input) {
        throw std::runtime_error("Failed to open template file: " + templatePath_);
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    templateContent_ = buffer.str();
}

void CertificateGenerator::loadRecipients() {
    std::ifstream dataFile(dataPath_);
    if (!dataFile) {
        throw std::runtime_error("Failed to open data file: " + dataPath_);
    }

    std::string line;
    bool headerSkipped = false;
    while (std::getline(dataFile, line)) {
        if (line.empty()) {
            continue;
        }
        if (!headerSkipped) {
            headerSkipped = true;
            continue;
        }

        auto columns = splitCSVLine(line);
        if (columns.size() < 4) {
            std::cerr << "Skipping malformed line: " << line << '\n';
            continue;
        }

        RecipientRecord record{columns[0], columns[1], columns[2], columns[3]};
        recipients_.push_back(std::move(record));
    }

    if (recipients_.empty()) {
        throw std::runtime_error("No recipients loaded from data file: " + dataPath_);
    }
}

void CertificateGenerator::ensureOutputDirectory() const {
    if (outputDirectory_.empty()) {
        throw std::runtime_error("Output directory is empty");
    }

    fs::path directory(outputDirectory_);
    if (!fs::exists(directory)) {
        fs::create_directories(directory);
    }
}

std::string CertificateGenerator::fillTemplate(const RecipientRecord &recipient) const {
    std::string filled = templateContent_;

    const std::vector<std::pair<std::string, std::string>> replacements = {
        {"{FULL_NAME}", recipient.fullName},
        {"{ACHIEVEMENT}", recipient.achievement},
        {"{DATE}", recipient.issueDate},
        {"{NOTES}", recipient.additionalNotes},
        {"{BACKGROUND}", style_.backgroundDescription},
        {"{FONT}", style_.fontDescription},
        {"{STYLE_NAME}", style_.name}};

    for (const auto &replacement : replacements) {
        std::string::size_type pos = 0;
        while ((pos = filled.find(replacement.first, pos)) != std::string::npos) {
            filled.replace(pos, replacement.first.length(), replacement.second);
            pos += replacement.second.length();
        }
    }

    return filled;
}

void CertificateGenerator::saveCertificate(const RecipientRecord &recipient, const std::string &content) const {
    auto fileName = sanitizeFileName(recipient.fullName) + ".txt";
    fs::path outputPath = fs::path(outputDirectory_) / fileName;

    std::ofstream outputFile(outputPath);
    if (!outputFile) {
        throw std::runtime_error("Failed to write certificate file: " + outputPath.string());
    }

    outputFile << content;
    std::cout << "Generated certificate for " << recipient.fullName
              << " -> " << outputPath << '\n';
}

void CertificateGenerator::saveMetadataReport() const {
    fs::path reportPath = fs::path(outputDirectory_) / "metadata.csv";
    std::ofstream reportFile(reportPath);
    if (!reportFile) {
        throw std::runtime_error("Failed to write metadata report: " + reportPath.string());
    }

    reportFile << "Full Name;Achievement;Date;Notes;Style" << '\n';
    for (const auto &recipient : recipients_) {
        reportFile << std::quoted(recipient.fullName, '"', '"') << ';'
                   << std::quoted(recipient.achievement, '"', '"') << ';'
                   << recipient.issueDate << ';'
                   << std::quoted(recipient.additionalNotes, '"', '"') << ';'
                   << style_.name << '\n';
    }

    std::cout << "Saved metadata report to " << reportPath << '\n';
}

std::unordered_map<std::string, VisualStyle> loadVisualStyles(const std::string &directory) {
    std::unordered_map<std::string, VisualStyle> styles;

    for (const auto &entry : fs::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::ifstream file(entry.path());
        if (!file) {
            std::cerr << "Skipping style file: " << entry.path() << '\n';
            continue;
        }

        VisualStyle style;
        style.name = entry.path().stem().string();

        std::string line;
        while (std::getline(file, line)) {
            auto delimiterPos = line.find(':');
            if (delimiterPos == std::string::npos) {
                continue;
            }

            auto key = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);

            if (key == "background") {
                style.backgroundDescription = value;
            } else if (key == "font") {
                style.fontDescription = value;
            }
        }

        styles.emplace(style.name, style);
    }

    return styles;
}
