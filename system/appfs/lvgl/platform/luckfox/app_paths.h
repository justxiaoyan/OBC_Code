#ifndef __APP_PATHS_H__
#define __APP_PATHS_H__

/**
 * @brief 获取可执行文件所在目录（以 '/' 结尾，如 "/appfs/"）
 *        运行时通过 /proc/self/exe 解析，结果缓存，仅解析一次
 * @return 目录字符串（静态存储，勿修改/释放）；获取失败返回 NULL
 */
const char *app_exe_dir(void);

/**
 * @brief 把 "./xxx" 形式的相对路径解析成可执行文件目录下的绝对路径，
 *        其余形式（绝对路径等）原样返回
 * @return 路径字符串（静态轮转缓冲，可被后续调用覆盖，勿释放）
 * @note 适用于 fopen 等即用即取的场景；若需长期持有（如 lv_img_set_src），
 *       请自行拷贝到持久存储
 */
const char *app_resolve_path(const char *path);

#endif /* __APP_PATHS_H__ */
