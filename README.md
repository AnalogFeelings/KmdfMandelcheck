# :blue_square: KmdfMandelcheck
![GitHub repo size](https://img.shields.io/github/repo-size/analogfeelings/kmdfmandelcheck?label=Repo%20Size&style=flat-square)
![Lines of code](https://img.shields.io/tokei/lines/github/analogfeelings/kmdfmandelcheck?label=Total%20Lines&style=flat-square)
![GitHub issues](https://img.shields.io/github/issues/analogfeelings/kmdfmandelcheck?label=Issues&style=flat-square)
![GitHub pull requests](https://img.shields.io/github/issues-pr/analogfeelings/kmdfmandelcheck?label=Pull%20Requests&style=flat-square)
![GitHub](https://img.shields.io/github/license/analogfeelings/kmdfmandelcheck?style=flat-square)
![GitHub Repo stars](https://img.shields.io/github/stars/analogfeelings/kmdfmandelcheck?label=Stars&style=flat-square)
![GitHub commit activity](https://img.shields.io/github/commit-activity/m/analogfeelings/kmdfmandelcheck?label=Commit%20Activity&style=flat-square)

This is a simple 200 LOC driver that displays a bitmap on screen after a BSOD occurs.

It uses a modified merge of ReactOS's [display.h](https://github.com/reactos/reactos/blob/master/sdk/include/reactos/drivers/bootvid/display.h) and [bootvid.h](https://github.com/reactos/reactos/blob/master/sdk/include/reactos/drivers/bootvid/bootvid.h) to be able to work with bootvid.dll properly.

# :thinking: Examples

https://user-images.githubusercontent.com/51166756/165745429-e586719b-5f62-4fcd-9af0-697c6f76de2c.mp4

https://user-images.githubusercontent.com/51166756/165746027-57102521-9000-46ab-89fe-48e87dd5be98.mp4

# :package: Building

Here are instructions on how to build this driver.

## :link: Building BOOTVID.lib

Open up the Visual Studio developer prompt, `cd` to KmdfMandelcheck's root directory, and then run the following command.

```
lib /def:BOOTVID.def /machine:x64 /out:BOOTVID.lib
```

## :gear: Building Mandelcheck.sys

Open Visual Studio 2022 and select "Debug x64" or "Release x64". Build the solution, and you will have `Mandelcheck.sys` in the output folder.

# :runner: How To Run

Drop `Mandelcheck.sys` in your VM and in an elevated command prompt run the following command.

```
sc create Mandelcheck binPath=C:\Where\The\File\Is\Mandelcheck.sys type=kernel start=auto
```

> [!IMPORTANT]  
> This driver will not work under UEFI systems. You must use a VM booted into Legacy BIOS mode.

## :framed_picture: Building A Valid Bitmap

Open your bitmap file in GIMP, scale/crop it down to 640x480, and make it a 16 color indexed image. Export it as .bmp, and now open it in **Paint**. Save the image as `target.bmp` and place it in `C:\KmdfMandelcheck\` for the driver to work.

> [!IMPORTANT]  
> The driver will display a black screen if the bitmap is not 16 color, 4bpp.

## :drum: And Now...

Run `sc start Mandelcheck` in an elevated command prompt and create a BSOD. You can use [BSODMachine](https://github.com/AnalogFeelings/BSODMachine) for an easy way, or you can break and execute `.crash` in WinDBG. If you are using WinDBG, once a bugcheck occurs, you must enter `g` to continue execution, otherwise the system will be completely halted.

# :balance_scale: License

Licensed under the [GPL version 3.0](LICENSE.txt).
