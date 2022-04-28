# KmdfMandelcheck

This is a simple 200 LOC driver that displays a bitmap on screen after a BSOD occurs.

It uses a modified merge of ReactOS's [display.h](https://github.com/reactos/reactos/blob/master/sdk/include/reactos/drivers/bootvid/display.h) and [bootvid.h](https://github.com/reactos/reactos/blob/master/sdk/include/reactos/drivers/bootvid/bootvid.h) to be able to work with bootvid.dll properly.

⚠️ WARNING ⚠️ :: **This driver will not work under UEFI systems. You must use a VM booted into Legacy BIOS mode.**

# Examples

https://user-images.githubusercontent.com/51166756/165745429-e586719b-5f62-4fcd-9af0-697c6f76de2c.mp4

https://user-images.githubusercontent.com/51166756/165746027-57102521-9000-46ab-89fe-48e87dd5be98.mp4

## Building BOOTVID.lib

Open up the Visual Studio developer prompt, `cd` to KmdfMandelcheck's root directory, and then run `lib /def:BOOTVID.def /machine:x64 /out:BOOTVID.lib`.

## Building Mandelcheck.sys

Open Visual Studio 2022 (2019 *may* work) and select "Debug x64" or "Release x64". Build the solution, and you will have `Mandelcheck.sys` in the output folder.

# How To Run

Drop `Mandelcheck.sys` in your VM and in an elevated command prompt run `sc create Mandelcheck binPath=C:\Where\The\File\Is\Mandelchecker.sys type=kernel start=auto`.

## Building A Valid Bitmap

Open your bitmap file in GIMP, scale/crop it down to 640x480, and make it a 16 color indexed image. Export it as .bmp, and now open it in **Paint**. Save the image as `target.bmp` and place it in `C:\KmdfMandelcheck\` for the driver to work.

⚠️ WARNING ⚠️ :: THE DRIVER WILL DISPLAY A BLACK SCREEN IF THE BITMAP IS NOT 16 COLOR 4BPP.

## And Now...

Run `sc start Mandelcheck` in an elevated command prompt and create a BSOD. You can use [BSODMachine](https://github.com/AestheticalZ/BSODMachine) for an easy way, or you can break and execute `.crash` in WinDBG. If you are using WinDBG, once a bugcheck occurs, you must enter `g` to continue execution, otherwise the system will be completely halted.

# License

Licensed under GPL v3.
