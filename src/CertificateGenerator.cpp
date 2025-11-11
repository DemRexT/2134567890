#include "CertificateGenerator.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
=======

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

std::string escapePDFText(const std::string &text) {
    std::string escaped;
    escaped.reserve(text.size());

    for (char ch : text) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '(':
            escaped += "\\(";
            break;
        case ')':
            escaped += "\\)";
            break;
        case '\r':
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }

    return escaped;
}

std::string buildPDFContentStream(const std::vector<std::string> &lines) {
    std::ostringstream content;
    content << "BT\n";
    content << "/F1 28 Tf\n";
    content << "1 0 0 1 72 770 Tm\n";

    if (!lines.empty()) {
        content << '(' << escapePDFText(lines.front()) << ") Tj\n";
    } else {
        content << "() Tj\n";
    }

    if (lines.size() > 1) {
        content << "24 TL\n";
        content << "/F1 16 Tf\n";
        for (std::size_t i = 1; i < lines.size(); ++i) {
            content << "T*\n";
            if (!lines[i].empty()) {
                content << '(' << escapePDFText(lines[i]) << ") Tj\n";
            }
        }
    }

    content << "ET\n";
    return content.str();
}

std::string buildSimplePDF(const std::vector<std::string> &lines) {
    std::string contentStream = buildPDFContentStream(lines);

    std::string pdf = "%PDF-1.4\n%\xC7\xEC\x8F\xA2\n";
    std::vector<std::size_t> objectOffsets;

    auto appendObject = [&](int number, const std::string &body) {
        objectOffsets.push_back(pdf.size());
        pdf += std::to_string(number) + " 0 obj\n" + body + "\nendobj\n";
    };

    appendObject(1, "<< /Type /Catalog /Pages 2 0 R >>");
    appendObject(2, "<< /Type /Pages /Kids [3 0 R] /Count 1 >>");
    appendObject(3, "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>");
    appendObject(4, "<< /Type /Font /Subtype /Type1 /BaseFont /Times-Roman >>");

    std::ostringstream contentObject;
    contentObject << "<< /Length " << contentStream.size() << " >>\n";
    contentObject << "stream\n" << contentStream << "endstream";
    appendObject(5, contentObject.str());

    std::size_t xrefOffset = pdf.size();
    std::ostringstream xref;
    xref << "xref\n0 6\n";
    xref << "0000000000 65535 f \n";
    for (auto offset : objectOffsets) {
        xref << std::setw(10) << std::setfill('0') << offset << " 00000 n \n";
    }

    pdf += xref.str();
    pdf += "trailer\n<< /Size 6 /Root 1 0 R >>\n";
    pdf += "startxref\n" + std::to_string(xrefOffset) + "\n%%EOF\n";

    return pdf;
}
=======
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
    auto fileName = sanitizeFileName(recipient.fullName) + ".pdf";
    fs::path outputPath = fs::path(outputDirectory_) / fileName;

    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    if (lines.empty()) {
        lines.emplace_back();
    }

    std::string pdfData = buildSimplePDF(lines);

    std::ofstream outputFile(outputPath, std::ios::binary);
=======
    auto fileName = sanitizeFileName(recipient.fullName) + ".txt";
    fs::path outputPath = fs::path(outputDirectory_) / fileName;

    std::ofstream outputFile(outputPath);
    if (!outputFile) {
        throw std::runtime_error("Failed to write certificate file: " + outputPath.string());
    }

    outputFile.write(pdfData.data(), static_cast<std::streamsize>(pdfData.size()));
=======
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
