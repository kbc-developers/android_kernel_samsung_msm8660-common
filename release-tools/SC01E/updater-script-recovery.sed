assert(getprop("ro.product.device") == "SC-01E" || getprop("ro.build.product") == "SC-01E" ||
       getprop("ro.product.device") == "SC01E" || getprop("ro.build.product") == "SC01E");
ui_print("");
ui_print("");
ui_print("------------------------------------------------");
ui_print("@VERSION");
ui_print("  KBC Developer:");
ui_print("    HomuHomu");
ui_print("    Ma34s");
ui_print("    Sakuramilk");
ui_print("------------------------------------------------");
ui_print("");
show_progress(0.500000, 0);

ui_print("flashing recovery image...");
package_extract_file("recovery.img", "/dev/block/mmcblk0p22");
show_progress(0.100000, 0);

ui_print("flash complete. Enjoy!");
set_progress(1.000000);

