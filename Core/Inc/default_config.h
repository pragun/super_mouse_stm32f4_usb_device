constexpr uint8_t default_cfg_size = 64;

#ifndef DEF_CONFIG
extern uint8_t default_cfg[default_cfg_size];
extern void fill_default_cfg();
#endif

