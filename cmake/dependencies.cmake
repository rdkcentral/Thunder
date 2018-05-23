include(config)

map()
key(provision)
    map()
    kv(tag master)
    kv(url "git@github.com:Metrological/libprovision.git)")
    end()
end()
ans(external_dependecies)

json_write("${CMAKE_CURRENT_LIST_DIR}/Tracing.json" ${PLUGIN_CONFIG})

message("external_dependecies ${external_dependecies}")