if Project "bldr-gui" then
    Compile "src/**"
    Include "src"

    Import {
        "luajit",
        "sol2",
        "glfw",
        "glad",
        "imgui-glfw",
        "imgui-opengl",
        "tinyfiledialogs",
    }

    Artifact { "out/gui", type = "Console" }
end