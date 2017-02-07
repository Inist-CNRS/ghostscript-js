{
  "targets": [
    {
      "target_name": "ghostscript-js",
      "sources": [ "src/ghostscript.cpp" ],
      "link_settings": {
        "libraries": [ "<(module_root_dir)/src/ghostpdl/sobin/libgs.so" ]
      },
      "include_dirs" : [
        "<!(node -e \"require('nan')\")"
      ],
      "cflags": ["-Wall"],
      "cflags_cc": ["-Wall"]
    }
  ]
}
