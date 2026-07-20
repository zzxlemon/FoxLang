#include "far.hpp"
#include "../util/utils.hpp"
#include "bytecode.hpp"
#include <cstring>
#include <sys/stat.h>
#include <direct.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir _mkdir
#else
#include <unistd.h>
#endif

static void write32(std::vector<uint8_t>& data, uint32_t v) {
    data.push_back(static_cast<uint8_t>(v & 0xFF));
    data.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    data.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

static uint32_t read32(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 4 > data.size()) return 0;
    uint32_t v = static_cast<uint32_t>(data[offset]) |
                 (static_cast<uint32_t>(data[offset + 1]) << 8) |
                 (static_cast<uint32_t>(data[offset + 2]) << 16) |
                 (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;
    return v;
}

bool FarArchive::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot open archive: " << path << std::endl;
        return false;
    }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    entries.clear();
    size_t offset = 0;

    if (offset + 4 > data.size() || data[offset] != 'F' || data[offset+1] != 'o' ||
        data[offset+2] != 'x' || data[offset+3] != 'A') {
        std::cerr << "Invalid .far file: bad magic" << std::endl;
        return false;
    }
    offset += 4;

    uint32_t version = read32(data, offset);
    if (version != 1) {
        std::cerr << "Unsupported .far version: " << version << std::endl;
        return false;
    }

    uint32_t count = read32(data, offset);
    for (uint32_t i = 0; i < count; i++) {
        FarEntry entry;
        uint32_t nameLen = read32(data, offset);
        if (offset + nameLen > data.size()) {
            std::cerr << "Corrupt .far: name out of bounds" << std::endl;
            return false;
        }
        entry.name.assign(reinterpret_cast<const char*>(data.data() + offset), nameLen);
        offset += nameLen;

        uint32_t dataSize = read32(data, offset);
        if (offset + dataSize > data.size()) {
            std::cerr << "Corrupt .far: data out of bounds" << std::endl;
            return false;
        }
        entry.data.assign(data.begin() + offset, data.begin() + offset + dataSize);
        offset += dataSize;

        entries.push_back(entry);
    }

    std::cout << "Loaded " << count << " entries from " << path << std::endl;
    return true;
}

bool FarArchive::save(const std::string& path) const {
    std::vector<uint8_t> data;

    data.push_back('F'); data.push_back('o'); data.push_back('x'); data.push_back('A');
    write32(data, 1); // version
    write32(data, static_cast<uint32_t>(entries.size()));

    for (const auto& entry : entries) {
        write32(data, static_cast<uint32_t>(entry.name.size()));
        for (char c : entry.name) data.push_back(static_cast<uint8_t>(c));
        write32(data, static_cast<uint32_t>(entry.data.size()));
        data.insert(data.end(), entry.data.begin(), entry.data.end());
    }

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot create archive: " << path << std::endl;
        return false;
    }
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();

    std::cout << "Created " << path << " (" << data.size() << " bytes, " << entries.size() << " entries)" << std::endl;
    return true;
}

void FarArchive::addFile(const std::string& name, const std::vector<uint8_t>& data) {
    entries.push_back({name, data});
}

bool FarArchive::extractFile(const std::string& name, std::vector<uint8_t>& data) const {
    for (const auto& entry : entries) {
        if (entry.name == name) {
            data = entry.data;
            return true;
        }
    }
    return false;
}

bool FarArchive::hasFile(const std::string& name) const {
    for (const auto& entry : entries) {
        if (entry.name == name) return true;
    }
    return false;
}

bool FarArchive::extractAll(const std::string& outputDir) const {
    // Create output directory
    mkdir(outputDir.c_str());

    for (const auto& entry : entries) {
        std::string filePath = outputDir + "/" + entry.name;
        // Create subdirectories if needed
        size_t pos = 0;
        while ((pos = filePath.find_first_of("/\\", pos + 1)) != std::string::npos) {
            std::string dir = filePath.substr(0, pos);
            mkdir(dir.c_str());
        }

        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Cannot extract: " << filePath << std::endl;
            return false;
        }
        file.write(reinterpret_cast<const char*>(entry.data.data()), entry.data.size());
        file.close();
        std::cout << "  extracted: " << filePath << " (" << entry.data.size() << " bytes)" << std::endl;
    }

    std::cout << "Extracted " << entries.size() << " files to " << outputDir << "/" << std::endl;
    return true;
}

bool farPackage(const std::string& outputPath, const std::vector<std::string>& inputFiles) {
    FarArchive archive;
    for (const auto& file : inputFiles) {
        std::ifstream in(file, std::ios::binary);
        if (!in.is_open()) {
            std::cerr << "Cannot read file: " << file << std::endl;
            return false;
        }
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();

        // Get filename without path
        std::string name = file;
        size_t sep = name.find_last_of("/\\");
        if (sep != std::string::npos) name = name.substr(sep + 1);

        archive.addFile(name, data);
        std::cout << "  added: " << name << " (" << data.size() << " bytes)" << std::endl;
    }
    return archive.save(outputPath);
}

bool farUnpack(const std::string& archivePath, const std::string& outputDir) {
    FarArchive archive;
    if (!archive.load(archivePath)) return false;
    return archive.extractAll(outputDir);
}

bool farRun(const std::string& archivePath) {
    FarArchive archive;
    if (!archive.load(archivePath)) return false;

    std::vector<uint8_t> fcData;
    if (!archive.extractFile("main.fc", fcData)) {
        // Try other .fc files
        for (const auto& entry : archive.entries) {
            if (entry.name.size() > 3 &&
                entry.name.substr(entry.name.size() - 3) == ".fc") {
                fcData = entry.data;
                std::cout << "Running: " << entry.name << std::endl;
                break;
            }
        }
        if (fcData.empty()) {
            std::cerr << "No .fc file found in archive" << std::endl;
            return false;
        }
    }

    try {
        CompiledProgram prog = CompiledProgram::deserialize(fcData);
        VM vm;
        vm.loadProgram(prog);
        vm.run();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[RuntimeError] " << e.what() << std::endl;
        return false;
    }
}
