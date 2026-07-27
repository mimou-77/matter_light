idf_component_register(SRC_DIRS          "." "matter" "app" "../hal"
                       PRIV_INCLUDE_DIRS  "."
                       "matter"
                       "app"
                       "../hal"
                       "${ESP_MATTER_PATH}/examples/common/utils"
                       "$ENV{IDF_PATH}/components/openthread/include"
                       "$ENV{IDF_PATH}/components/openthread/openthread/include" )

target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")

