
### Layers

- app_main : __app_main.cpp__

- app_logic_matter : __app_logic_matter.cpp__
  - the same for all matter projects
  - impl of `app_attribute_update_cb()` : 

- app_driver_matter : __app_driver_matter.cpp__
  - impl of `app_driver_attribute_update()`
  - app_driver_handle (exp: led_handle, relay_handle, ...) : 

- x_driver_matter : __x_driver_matter.cpp__
  - impl of `create_led(node_t * led_node, uint8_t led_pin)`(creates endpoint and attributes)

- hal_led : __hal_led.cpp__                                             

- hal_gpio = __driver/gpio.h__ 



<table style="border-collapse: collapse; width: 40%; margin-left: 0; text-align: left; font-family: sans-serif; color: black;">
  <tbody>
    <tr style="background-color: #eeeeee;">
      <td colspan="2" style="border: 1px solid black; padding: 8px; color: black;">app_main</td>
    </tr>
    <tr>
      <td rowspan="2" style="border: 1px solid black; padding: 8px; background-color: #ffe599; color: black; width: 50%;">app_driver_matter</td>
      <td style="border: 1px solid black; padding: 8px; background-color: #b6d7a8; color: black;">app_logic_matter</td>
    </tr>
    <tr>
      <td style="border: 1px solid black; padding: 8px; background-color: #ffe599; color: black;">app_driver_matter</td>
    </tr>
    <tr style="background-color: #f6b26b;">
      <td style="border: 1px solid black; padding: 8px; color: black;">x_driver_matter</td>
      <td style="border: 1px solid black; padding: 8px; color: black;">y_driver_matter</td>
    </tr>
    <tr style="background-color: #f4cccc;">
      <td colspan="2" style="border: 1px solid black; padding: 8px; color: black;">hal_led</td>
    </tr>
  </tbody>
</table>

<br>

---

<br>

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

<br>

---

<br>

> [!Warning] esp-idf v5.2.1 && esp-matter main
> 