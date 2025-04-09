#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <regex>
#include "ModelTagging.h"
#include <QProcess>
#include <QDebug>
#include <QtConcurrent>
#include <QFuture>
#include "executeCommand.h"

void logToFile(const std::string& message) {
    static std::mutex logMutex; 
    std::lock_guard<std::mutex> lock(logMutex);

    std::ofstream logFile("ai_tagging_log.txt", std::ios_base::app); 
    if (logFile.is_open()) {
        std::time_t now = std::time(nullptr);
        char* dt = std::ctime(&now);
        dt[strlen(dt) - 1] = '\0';
        logFile << "[" << dt << "] " << message << "\n";
    }
}

bool ModelTagging::checkOllamaAvailability() {
#ifdef _WIN32
    // Windows command using our blocking function (consider revising this later too)
    std::string output = executeCommandNoWindow("cmd /C where ollama");
    logToFile("checkOllamaAvailability: output = " + output);
    int result = output.find("ollama") == std::string::npos;
    logToFile("checkOllamaAvailability: result = " + std::to_string(result));

#else
    int result = std::system("which ollama >/dev/null 2>&1");
    logToFile("checkOllamaAvailability (Linux/mac): result = " + std::to_string(result));
#endif
    return result == 0;
}
 
bool ModelTagging::checkModelAvailability(const std::string& modelName) {
#ifdef _WIN32
    std::string output = executeCommandNoWindow("cmd /C ollama list");
    logToFile("checkModelAvailability: raw output = " + output);
    bool found = output.find(modelName) != std::string::npos;
    logToFile("checkModelAvailability: model \"" + modelName + "\" found = " + std::to_string(found));
    return found;
#else
    std::string command = "ollama list | grep -q \"" + modelName + "\"";
    int result = std::system(command.c_str());
    logToFile("checkModelAvailability (Linux/mac): result = " + std::to_string(result));
    return (result == 0);
#endif
}

ModelTagging::ModelTagging(){
    // instantiate the parser
    parser = ModelParser();
}

ModelTagging::~ModelTagging(){
    // destructor implementation
    if (tagProcess) {
        tagProcess->kill();
        tagProcess->deleteLater();
    }
}

void ModelTagging::generateTags(const std::string& filepath) {
	m_generationCanceled = false; // Reset the cancellation 

    logToFile("generateTags() called with: " + filepath);

    auto blockingTask = [this, filepath]() -> bool {
        // Check if Ollama is available.
        if (!checkOllamaAvailability()) {
            std::cerr << "ERROR: Ollama is not available." << std::endl;
            logToFile("ERROR: Ollama is not available.");
            return false;
        }
        // Check if the model is available.
        if (!checkModelAvailability(this->modelName)) {
            std::cerr << "ERROR: Model " << this->modelName << " is not available." << std::endl;
			logToFile("ERROR: Model " + this->modelName + " is not available.");
            return false;
        }

        // Parse metadata.
        logToFile("Ollama and model available. Parsing metadata...");
        ModelMetadata metadata = parser.parseModel(filepath);
        logToFile("Metadata parsed. Title: " + metadata.title + ", Object count: " + std::to_string(metadata.objectFiles.size()));

        // Build the prompt.
        std::ostringstream objectPathsStream;
        for (const auto& object : metadata.objectFiles) {
            objectPathsStream << "  - " << object << "\n";
        }
        std::ostringstream prompt;
        prompt << R"(
            You are an expert CAD modeler organizing CAD files for an engineering database.
            
            Generate exactly 10 relevant tags for categorization and search filtering of this 3D CAD model.
            
            ### **File Metadata:**
            - **Filepath:** )" << filepath << R"(
            - **Title:** )" << metadata.title << R"(
            - **Object Paths:**  
        )" << objectPathsStream.str() << R"(

            ### **Instructions:**
            - **ONLY use words directly related to the object in the CAD model.**  
            - **Do NOT include metadata like names, file paths, BRL-CAD, or dates.**  
            - **Prioritize useful search terms for categorization (e.g., mechanical, architectural, vehicle, tool).**  
            - **Infer object identity from file names, object names, and folder structure.**  
            - **Do NOT make them generic (e.g., object, model, 3D).**
            - **Do NOT assume additional details beyond what's in the metadata.**
            - **Your output must be EXACTLY 10 single-word tags, one per line with no numbering or additional formatting.**

            ### Example:
            For a teapot model, appropriate tags would be:
            Teapot
            Container
            Lid
            Ceramic
            Tableware
            Kitchenware
            Beverage
            Drinking
            Serving
            Decorative
        )";

        // Write the prompt to a temporary file.
        std::ofstream promptFile("prompt.txt");
        if (!promptFile.is_open()) {
            logToFile("ERROR: Could not open prompt.txt for writing.");
            std::cerr << "ERROR: Could not open prompt.txt for writing." << std::endl;
            return false;
        }
        promptFile << prompt.str();
        promptFile.close();
        logToFile("Prompt written to prompt.txt");

        return true;
        };

    QFuture<bool> future = QtConcurrent::run(blockingTask);
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher]() {
        bool success = watcher->result();
        watcher->deleteLater();

        if (m_generationCanceled) {
            logToFile("Generation canceled during blocking tasks.");
            qDebug() << "Generation was canceled during blocking tasks.";
            emit tagGenerationCanceled();
            return;
        }

        if (!success) {
            logToFile("Blocking task failed. Emitting empty tags.");
            emit tagsGenerated({});
            return;
        }

        logToFile("Starting Ollama QProcess...");
        if (tagProcess) {
            tagProcess->deleteLater();
        }
        tagProcess = new QProcess(this);
        connect(tagProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ModelTagging::onTagProcessFinished);

        tagProcess->setProgram("ollama");
        tagProcess->setArguments(QStringList() << "run" << "llama3");
        tagProcess->setStandardInputFile("prompt.txt");
        tagProcess->setStandardOutputFile("temp_tags.txt");

        tagProcess->start();
        logToFile("QProcess started with: ollama run llama3");
     });

    watcher->setFuture(future);
}

void ModelTagging::cancelTagGeneration() {
	m_generationCanceled = true;
    if (tagProcess && tagProcess->state() == QProcess::Running) {
        tagProcess->terminate();
        if (!tagProcess->waitForFinished(2000)) {
            tagProcess->kill();
        }
        qDebug() << "Tag generation canceled (process killed).";
        emit tagGenerationCanceled();
    }
    else {
        qDebug() << "Tag generation canceled before process started.";
        emit tagGenerationCanceled();
    }
}

void ModelTagging::onTagProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qDebug() << "[ModelTagging] onTagProcessFinished: exitCode=" << exitCode
        << "exitStatus=" << exitStatus;
    logToFile("onTagProcessFinished: exitCode = " + std::to_string(exitCode) +
        ", exitStatus = " + std::to_string(exitStatus));
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);

    std::vector<std::string> tags;
    std::ifstream file("temp_tags.txt");
	if (!file.is_open()) {
        logToFile("ERROR: Failed to open temp_tags.txt");
	}
    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    file.close();
    content = std::regex_replace(content, std::regex(R"(\*\*)"), "");

    logToFile("Raw output from Ollama:\n" + content);

    std::regex tagPattern(R"((?:^|\n)([A-Za-z]+)(?:\n|$))");
    std::sregex_iterator it(content.begin(), content.end(), tagPattern);
    std::sregex_iterator end;
    int count = 0;
    while (it != end && count < 10) {
        std::string tag = (*it)[1];
        if (!tag.empty()) {
            tags.push_back(tag);
            count++;
        }
        ++it;
    }
    if (tags.size() < 10) {
        logToFile("Tag count less than 10. Fallback parsing used.");
        std::istringstream contentStream(content);
        std::string line;
        tags.clear();
        while (std::getline(contentStream, line) && tags.size() < 10) {
            line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
            line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);
            if (line.empty() || line.find(' ') != std::string::npos || !std::isalpha(line[0])) {
                continue;
            }
            tags.push_back(line);
        }
    }

    QFile promptFile("prompt.txt");
    if (promptFile.exists()) {
        promptFile.remove();
    }

    QFile tagsFile("temp_tags.txt");
    if (tagsFile.exists()) {
        tagsFile.remove();
    }

    // Emit the signal with the generated tags.
    emit tagsGenerated(tags);

    qDebug() << "[ModelTagging] tagsGenerated signal emitted with "
        << tags.size() << "tags";
    logToFile("Final tag count: " + std::to_string(tags.size()));

    tagProcess->deleteLater();
    tagProcess = nullptr;
}
