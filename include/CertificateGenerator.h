#ifndef CERTIFICATE_GENERATOR_H
#define CERTIFICATE_GENERATOR_H

#include <string>
#include <vector>
#include <unordered_map>

struct RecipientRecord {
    std::string fullName;
    std::string achievement;
    std::string issueDate;
    std::string additionalNotes;
};

struct VisualStyle {
    std::string name;
    std::string backgroundDescription;
    std::string fontDescription;
};

class CertificateGenerator {
public:
    CertificateGenerator(std::string templatePath,
                         std::string dataPath,
                         std::string outputDirectory,
                         VisualStyle style);

    void run();

private:
    std::string templatePath_;
    std::string dataPath_;
    std::string outputDirectory_;
    VisualStyle style_;

    std::string templateContent_;
    std::vector<RecipientRecord> recipients_;

    void loadTemplate();
    void loadRecipients();
    void ensureOutputDirectory() const;
    std::string fillTemplate(const RecipientRecord &recipient) const;
    void saveCertificate(const RecipientRecord &recipient, const std::string &content) const;
    void saveMetadataReport() const;
};

std::unordered_map<std::string, VisualStyle> loadVisualStyles(const std::string &directory);

#endif // CERTIFICATE_GENERATOR_H
