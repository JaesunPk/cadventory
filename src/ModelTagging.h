#ifndef MODELTAGGING_H
#define MODELTAGGING_H

#include <string>
#include <vector>
#include "ModelParser.h"
#include <QProcess>
#include <QObject>
#include "ModelMetadata.h"

/**
 * @brief main class for handling model tags.
 * 
 * This class will generate a list of tags for a model after given a set prompt.abs
 * The prompt will consist of the file path to the model itself, the title, and the object files that are used to generate the model.
 * This is known as the model's metadata. The extraction of model data will be through the ModelParser class.
 * 
 * How to use:
 * - simply create and instance of the class and call generateTags(std::string filepath) to generate the tags.
 */
class ModelTagging : public QObject {
    Q_OBJECT
public:
    ModelTagging();

    ~ModelTagging();

    void generateTags(const std::string& filepath);
    void cancelTagGeneration();

signals:
    void tagsGenerated(const std::vector<std::string>& tags);
    void tagGenerationCanceled();

private slots:
    void onTagProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    bool checkOllamaAvailability();
    bool checkModelAvailability(const std::string& modelName);
    std::string modelName = "llama3"; // default model name - change if needed
    ModelParser parser;
    QProcess* tagProcess = nullptr;
};

#endif // MODELTAGGING_H