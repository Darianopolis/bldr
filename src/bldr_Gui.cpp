#include <bldr_Server.hpp>

#ifndef GLFW_INCLUDE_NONE
#  define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <tinyfiledialogs.h>

namespace bldr
{
    struct BldrGui
    {
        GLFWwindow* window;

        ProjectDatabase project_database;

        std::vector<CCppProject*> select_history;
        CCppProject*              selected_project = nullptr;

        void Init()
        {
            glfwInit();
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            window = glfwCreateWindow(1920, 1080, "bldr", nullptr, nullptr);

            glfwMakeContextCurrent(window);
            gladLoadGL(glfwGetProcAddress);
            glfwSwapInterval(1);

            ImGui::CreateContext();

            auto& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

            ImGui_ImplGlfw_InitForOpenGL(window, true);
            ImGui_ImplOpenGL3_Init("#version 330 core");
        }

        void BeginFrame()
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Make dockspace fullscreen
            auto viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

            ImGuiDockNodeFlags dockspace_flags = 0;
            ImGuiWindowFlags dockspace_window_flags = 0
                | ImGuiWindowFlags_NoTitleBar
                | ImGuiWindowFlags_NoCollapse
                | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoBringToFrontOnFocus
                | ImGuiWindowFlags_NoNavFocus
                | ImGuiWindowFlags_MenuBar;

            // Register dockspace
            bool open = true;
            ImGui::Begin("Dockspace", &open, dockspace_window_flags);
            ImGui::PopStyleVar(3);
            ImGui::DockSpace(ImGui::GetID("DockspaceID"), ImVec2(0.f, 0.f), dockspace_flags);

            // TODO: Menu bar contents

            ImGui::End();
        }

        void EndFrame()
        {
            ImGui::Render();

            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        void ProjectExplorer()
        {
            if (ImGui::Begin("Project Files")) {

                if (ImGui::Button("Add file")) {
                    auto file = tinyfd_openFileDialog(
                        "Load bldr file",
                        fs::current_path().string().c_str(),
                        2, std::array<const char*, 2>{"bldr.lua", "*.bldr.lua"}.data(),
                        "bldr script files",
                        false);
                    if (file) {
                        if (std::ranges::find(project_database.project_files, std::string(file)) == project_database.project_files.end()) {
                            project_database.project_files.emplace_back(file);
                        }
                    }
                }

                ImGui::SameLine();
                if (ImGui::Button("Add test files")) {
                    project_database.project_files.emplace_back("D:/Dev/cloned.bldr.lua");
                    project_database.project_files.emplace_back("D:/Dev/Projects/nova/bldr.lua");
                    project_database.project_files.emplace_back("D:/Dev/Projects/bldr-gui/bldr.lua");
                }

                ImGui::SameLine();
                if (ImGui::Button("Clear")) {
                    project_database.project_files.clear();
                }

                ImGui::BeginChild(ImGui::GetID("project files"));

                ImGuiListClipper clipper;
                clipper.Begin(int(project_database.project_files.size()));
                bool break_early = false;
                while (clipper.Step()) {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                        auto& project_file = project_database.project_files[i];

                        bool selected = false;
                        if (ImGui::Selectable(project_file.c_str(), &selected)) {
                            std::println("Selected: {}", project_file);
                        }

                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }

                        if (ImGui::BeginPopupContextItem()) {

                            if (ImGui::Button("Remove")) {
                                std::erase(project_database.project_files, project_file);
                                break_early = true;
                                ImGui::CloseCurrentPopup();
                            }

                            ImGui::EndPopup();
                        }
                        if (break_early) break;
                    }
                    if (break_early) break;
                }

                ImGui::EndChild();
            }
            ImGui::End();

            if (ImGui::Begin("Available projects")) {
                if (ImGui::Button("Update")) {
                    if (selected_project) {
                        auto last_selected_name = selected_project->name;
                        project_database.LoadProjects();
                        selected_project = project_database.FindProjectForName(last_selected_name);
                    } else {
                        project_database.LoadProjects();
                    }
                    select_history.clear();
                }

                ImGui::BeginChild(ImGui::GetID("projects"));

                ImGuiListClipper clipper;
                clipper.Begin(int(project_database.c_cpp_projects.size()));
                bool break_early = false;
                while (clipper.Step()) {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                        auto& project = project_database.c_cpp_projects[i];

                        bool selected = false;
                        if (ImGui::Selectable(project->name.c_str(), &selected)) {
                            std::println("Selected: {}", project->name);
                            selected_project = project.get();
                            select_history.clear();
                        }

                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }

                        if (break_early) break;
                    }
                    if (break_early) break;
                }

                ImGui::EndChild();
            }
            ImGui::End();

            if (ImGui::Begin("Project details")) do {
                if (!selected_project) {
                    ImGui::Text("No selected project");
                    break;
                }

                if (!select_history.empty()) {
                    if (ImGui::Button("<< Back")) {
                        selected_project = select_history.back();
                        select_history.pop_back();
                    }
                } else {
                    ImGui::BeginDisabled();
                    ImGui::Button("<< Back");
                    ImGui::EndDisabled();
                }

                ImGui::Separator();

                auto& project = *selected_project;
                ImGui::Text(std::format("Name: {}", project.name).c_str());
                ImGui::Text(std::format("Dir:  {}", project.dir.generic_string()).c_str());

                if (!project.import_names.empty()) {
                    ImGui::SeparatorText("Imports");

                    for (auto& import : project.import_names) {
                        auto i_proj = std::ranges::find_if(project_database.c_cpp_projects, [&](const auto& p) { return p->name == import; });
                        auto* import_proj = i_proj == project_database.c_cpp_projects.end() ? nullptr : i_proj->get();
                        bool selected = false;
                        if (ImGui::Selectable(import_proj ? std::format("- {}", import).c_str() : std::format("! {} (not found)", import).c_str(), &selected)) {
                            if (import_proj) {
                                select_history.push_back(selected_project);
                                selected_project = import_proj;
                            }
                        }
                    }
                }

                auto ListFiles = [&](const char* name, std::span<const FileRef> files) {
                    if (!files.empty()) {
                        ImGui::SeparatorText(name);

                        for (auto& file : files) {
                            if (file.type == FileType::Auto) {
                                ImGui::Text(std::format("- {}", file.path).c_str());
                            } else {
                                ImGui::Text(std::format("- {} ({})", file.path, FileTypeToString(file.type)).c_str());
                            }
                        }
                    }
                };

                ListFiles("Compile sources", project.sources);
                ListFiles("Embeds", project.embeds);

                ListFiles("Include Directories", project.includes);
                ListFiles("Force Includes", project.force_includes);

                ListFiles("Library Paths", project.lib_paths);
                ListFiles("Links", project.links);
                ListFiles("Shared Libraries", project.shared_libs);

            } while (0);
            ImGui::End();
        }

        void Run()
        {
            bool run_second_update = true;
            while (!glfwWindowShouldClose(window)) {

                BeginFrame();

                ImGui::ShowDemoWindow();
                ProjectExplorer();

                EndFrame();

                glfwSwapBuffers(window);
                if (run_second_update) {
                    glfwPollEvents();
                    run_second_update = false;
                } else {
                    glfwWaitEvents();
                    run_second_update = true;
                }
            }
        }

        ~BldrGui()
        {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();

            glfwTerminate();
        }
    };
}

int main()
{
    try {
        bldr::BldrGui gui;
        gui.Init();
        gui.Run();
    } catch (const std::exception& e)
    {
        std::println("Error: {}", e.what());
    }
}