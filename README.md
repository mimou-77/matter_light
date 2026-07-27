

> [!Note] How to build :
> `For wifi` : 
> ``` 
> idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.c6_wifi" build
>
> ```
> `For thread` : 
> ``` 
> idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.c6_thread" build
> ```
>
> each variant uses its own generated config file (`sdkconfig.c6_wifi` / `sdkconfig.c6_thread`,
> selected in the root CMakeLists.txt from SDKCONFIG_DEFAULTS) : without this, idf.py would
> silently reuse the existing `sdkconfig` and keep the previous transport.
> switching transport triggers a full rebuild (shared build dir).

<br>

---

<br>

> [!Warning] esp-idf v5.2.1 && esp-matter main
> 