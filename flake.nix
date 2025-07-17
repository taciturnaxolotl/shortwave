{
  description = "Win32 Hello World App with About dropdown";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.05";
    flake-parts.url = "github:hercules-ci/flake-parts";
    systems.url = "github:nix-systems/default";
  };

  outputs = inputs@{ flake-parts, systems, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [ ];
      systems = import systems;
      perSystem = { self', pkgs, ... }: {
        packages = {
          hello-world-app = pkgs.stdenv.mkDerivation {
            name = "hello-world-app";
            version = "1.0.0";
            src = ./.;
            
            nativeBuildInputs = with pkgs; [
              cmake
              pkgsCross.mingw32.buildPackages.gcc
            ];
            
            buildInputs = with pkgs.pkgsCross.mingw32; [
              windows.mingw_w64
            ];
            
            configurePhase = ''
              export CC=${pkgs.pkgsCross.mingw32.buildPackages.gcc}/bin/i686-w64-mingw32-gcc
              export CXX=${pkgs.pkgsCross.mingw32.buildPackages.gcc}/bin/i686-w64-mingw32-g++
              export CMAKE_SYSTEM_NAME=Windows
              export CMAKE_C_COMPILER=$CC
              export CMAKE_CXX_COMPILER=$CXX
            '';
            
            buildPhase = ''
              cmake -DCMAKE_BUILD_TYPE=Release \
                    -DCMAKE_SYSTEM_NAME=Windows \
                    -DCMAKE_C_COMPILER=$CC \
                    -DCMAKE_CXX_COMPILER=$CXX .
              make VERBOSE=1
            '';
            
            installPhase = ''
              mkdir -p $out/bin
              cp HelloWorld.exe $out/bin/
            '';
          };
          
          deploy-to-xp = pkgs.writeShellScriptBin "deploy-to-xp" ''
            XP_DIR="$HOME/Documents/xp-drive"
            mkdir -p "$XP_DIR"
            cp ${self'.packages.hello-world-app}/bin/HelloWorld.exe "$XP_DIR/"
            echo "Deployed HelloWorld.exe to $XP_DIR"
          '';
          
          setup-dev = pkgs.writeShellScriptBin "setup-dev" ''
            echo "Setting up development environment for Zed..."
            
            # Get the proper MinGW headers - use the known path from our build
            MINGW_MAIN_INCLUDE="/nix/store/hhbkp872dkayzd2qxfhkdc4rgn393g52-mingw-w64-i686-w64-mingw32-9.0.0-dev/include"
            
            # Verify it exists, if not try to find it dynamically
            if [ ! -f "$MINGW_MAIN_INCLUDE/windows.h" ]; then
              echo "Static path not found, searching dynamically..."
              MINGW_MAIN_INCLUDE=$(i686-w64-mingw32-gcc -v -E - < /dev/null 2>&1 | sed -n '/mingw-w64.*-dev\/include/p' | head -1 | awk '{print $2}')
              
              if [ -z "$MINGW_MAIN_INCLUDE" ] || [ ! -f "$MINGW_MAIN_INCLUDE/windows.h" ]; then
                echo "Error: Could not find proper MinGW headers with windows.h"
                exit 1
              fi
            fi
            
            # Create simplified .clangd config to avoid intrinsics issues
            cat > .clangd << EOF
            CompileFlags:
              Add:
                - -target
                - i686-w64-mingw32
                - -DWINVER=0x0501
                - -D_WIN32_WINNT=0x0501
                - -DWIN32_LEAN_AND_MEAN
                - -D_WIN32
                - -DWIN32
                - -std=c++17
                - -fno-builtin
                - -D__NO_INLINE__
                - -isystem
                - $MINGW_MAIN_INCLUDE
              Remove:
                - -I*/gcc/*/include
            EOF
            
            # Create simplified compile_commands.json
            cat > compile_commands.json << EOF
            [
              {
                "directory": "$(pwd)",
                "command": "clang++ -target i686-w64-mingw32 -DWINVER=0x0501 -D_WIN32_WINNT=0x0501 -DWIN32_LEAN_AND_MEAN -D_WIN32 -DWIN32 -fno-builtin -isystem \"$MINGW_MAIN_INCLUDE\" -std=c++17 -c main.cpp",
                "file": "main.cpp"
              }
            ]
            EOF
            
            echo "Generated simplified .clangd config and compile_commands.json"
            echo "Using MinGW headers: $MINGW_MAIN_INCLUDE"
            echo ""
            echo "This avoids GCC intrinsics that cause clang issues"
            echo "Restart Zed for the changes to take effect"
          '';
          
          default = self'.packages.hello-world-app;
        };
        
        devShells = {
          default = pkgs.mkShell {
            buildInputs = with pkgs; [
              cmake
              pkgsCross.mingw32.buildPackages.gcc
              self'.packages.deploy-to-xp
              self'.packages.setup-dev
            ];
            
            shellHook = ''
              echo "Win32 development environment loaded (older toolchain)"
              echo "Available commands:"
              echo "  nix build     - Build the application"
              echo "  deploy-to-xp  - Deploy to XP VM folder"
              echo "  setup-dev     - Generate compile_commands.json for Zed editor"
              echo ""
              echo "For Zed editor support:"
              echo "  1. Run 'setup-dev' to generate compile_commands.json"
              echo "  2. Open project in Zed for IntelliSense support"
            '';
          };
        };
      };
    };
}