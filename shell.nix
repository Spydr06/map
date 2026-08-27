{ pkgs ? import <nixpkgs> {} }:
pkgs.stdenvNoCC.mkDerivation {
    name = "map-env";

    buildInputs = with pkgs; [
        # toolchain
        bear
        cmake
        gcc16
        gdb
        gf
        gnumake
        pkg-config
        valgrind

        # libraries
        cairo
        curl
        glew
        glfw
        glm
        gtk2
        gtk3
        gtk4
        json_c
        libGL
        libarchive
        librsvg
        glib
        glib.dev

        # cartography
        gdal
        josm
        libgeotiff
        libtiff
        proj
    ];

    hardeningDisable = [ "all" ];

    shellHook = ''
        export LD_LIBRARY_PATH="''${LD_LIBRARY_PATH}''${LD_LIBRARY_PATH:+:}${pkgs.libglvnd}/lib"
        export XDG_DATA_DIRS="$XDG_DATA_DIRS:${pkgs.gtk3}/share/gsettings-schemas/${pkgs.gtk3.name}"
        unset SOURCE_DATE_EPOCH
        unset DETERMINISTIC_BUILD
    '';
}

