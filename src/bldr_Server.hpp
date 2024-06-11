#pragma once

#include <bldr_Core.hpp>

namespace bldr
{
    inline
    void Error(std::string msg)
    {
        throw std::runtime_error(msg);
    }

    template<class... Args>
    void Error(const std::format_string<Args...> fmt, Args&&... args)
    {
        throw std::runtime_error(std::vformat(fmt.get(), std::make_format_args(args...)));
    }

    enum class FileType
    {
        Auto,
        C,
        Cpp,
        CppModule,
        Slang,
    };

    inline
    FileType FileTypeFromString(std::string_view ext)
    {
        if (ext == "c")     return FileType::C;
        if (ext == "cpp")   return FileType::Cpp;
        if (ext == "cppm")  return FileType::CppModule;
        if (ext == "slang") return FileType::Slang;
        Error("Unrecognized file type: {} (Expected 'c', 'cpp', 'cppm', or 'slang')", ext);
    }

    inline
    std::string_view FileTypeToString(FileType type)
    {
        switch (type) {
            case FileType::Auto:      return "Auto";
            case FileType::C:         return "C";
            case FileType::Cpp:       return "C++";
            case FileType::CppModule: return "C++ Modules";
            case FileType::Slang:     return "Slang";
        }
        Error("Invalid FileType enum value");
    }

    struct FileRef
    {
        std::string path;
        FileType    type = FileType::Auto;
    };

    struct CStyleDefine
    {
        std::string name;
        std::string value;
    };

    enum class CCppArtifactType
    {
        ConsoleExecutable,
        WindowExecutable,
        SharedLibrary,
    };

    struct CCppArtifact
    {
        std::string      file;
        CCppArtifactType type;
    };

    struct CCppProject
    {
        std::string name;
        fs::path    dir;

        std::vector<std::string> import_names;

        std::vector<FileRef> sources;
        std::vector<FileRef> embeds;
        std::vector<FileRef> includes;
        std::vector<FileRef> force_includes;
        std::vector<FileRef> lib_paths;
        std::vector<FileRef> links;
        std::vector<FileRef> shared_libs;

        std::vector<CStyleDefine> build_defines;
        std::vector<CStyleDefine> defines;

        std::optional<CCppArtifact> artifact;
    };

    struct ProjectDatabase
    {
        std::vector<std::unique_ptr<CCppProject>> c_cpp_projects;

        std::vector<std::string> project_files;

        void LoadProjects();
        CCppProject* FindProjectForName(std::string_view name);
    };
}