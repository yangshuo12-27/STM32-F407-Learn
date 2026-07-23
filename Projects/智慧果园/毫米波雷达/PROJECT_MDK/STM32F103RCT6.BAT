SET PATH=D:\App\Keil_v5\ARM\ARMCC\Bin;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\Windows\System32\OpenSSH\;C:\Program Files\dotnet\;C:\Users\27327\AppData\Local\Microsoft\WindowsApps;
SET CPU_TYPE=STM32F103RC
SET CPU_VENDOR=STMicroelectronics
SET UV2_TARGET=STM32F103RCT6
SET CPU_CLOCK=0x00B71B00
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\delay.__i"
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\key.__i"
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\led.__i"
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\sys.__i"
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\timer.__i"
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\usart.__i"
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\font.__i"
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\esp8266.__i"
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\lcd.__i"
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\mlx90614.__i"
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\main.__i"
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\core_cm3.__i"
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\system_stm32f10x.__i"
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmAsm" --Via ".\objects\startup_stm32f10x_hd._ia"
"D:\App\Keil_v5\ARM\ARMCC\Bin\ArmLink" --Via ".\Objects\STM32HD.lnp"
"D:\App\Keil_v5\ARM\ARMCC\Bin\fromelf.exe" ".\Objects\STM32HD.axf" --i32combined --output ".\Objects\STM32HD.hex"
