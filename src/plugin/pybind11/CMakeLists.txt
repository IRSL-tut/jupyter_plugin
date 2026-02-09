set(target PyJupyterPlugin)

choreonoid_add_python_module(${target}
  PyJupyterPluginModule.cpp
)

target_link_libraries(${target} CnoidJupyterPlugin)
