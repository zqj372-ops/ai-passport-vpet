idf_component_register(
    SRCS "main.c"
         "ui_pixel.c"
         "ui_pixel_math.c"
         "vpet_app.c"
         "vpet_audio.c"
         "vpet_battle.c"
         "vpet_link.c"
         "vpet_model.c"
         "vpet_species.c"
         "vpet_sprite.c"
         "vpet_store.c"
         "vpet_view.c"
         "vpet_font_16.c"
    INCLUDE_DIRS "."
    REQUIRES bsp bt nvs_flash esp_timer
)
