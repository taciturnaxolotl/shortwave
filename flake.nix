{
  description = "Shortwave Radio Tuner - Win32 App for Windows XP";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.05";
    flake-parts.url = "github:hercules-ci/flake-parts";
    systems.url = "github:nix-systems/default";
  };

  outputs =
    inputs@{ flake-parts, systems, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [ ];
      systems = import systems;
      perSystem =
        { self', pkgs, ... }:
        {
          packages = {
            shortwave = pkgs.stdenv.mkDerivation {
              name = "shortwave";
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
                export LDFLAGS="-static -static-libgcc -static-libstdc++"
                
                # Check if BASS files exist in libs directory
                if [ -f "libs/bass.h" ] && [ -f "libs/bass.lib" ]; then
                  echo "BASS files found in libs/ - building with audio integration"
                else
                  echo "BASS files not found in libs/ - building without audio integration"
                  # Disable BASS in the source
                  sed -i 's|#include "libs/bass.h"|// #include "libs/bass.h"|' main.cpp
                  sed -i 's|libs/bass.lib||' CMakeLists.txt
                fi
                
                cmake -DCMAKE_BUILD_TYPE=Release \
                      -DCMAKE_SYSTEM_NAME=Windows \
                      -DCMAKE_C_COMPILER=$CC \
                      -DCMAKE_CXX_COMPILER=$CXX \
                      -DCMAKE_EXE_LINKER_FLAGS="$LDFLAGS" .
                make VERBOSE=1
              '';

              installPhase = ''
                mkdir -p $out/bin
                cp Shortwave.exe $out/bin/
              '';
            };

            deploy-to-xp = pkgs.writeShellScriptBin "deploy-to-xp" ''
              XP_DIR="$HOME/Documents/xp-drive"
              mkdir -p "$XP_DIR"

              echo "Deploying Shortwave Radio to $XP_DIR..."

              # Copy executable with force overwrite
              if cp -f ${self'.packages.shortwave}/bin/Shortwave.exe "$XP_DIR/"; then
                echo "✓ Copied Shortwave.exe"
              else
                echo "✗ Failed to copy Shortwave.exe"
                exit 1
              fi

              # Copy BASS DLL from libs directory
              if [ -f libs/bass.dll ]; then
                if cp -f libs/bass.dll "$XP_DIR/"; then
                  echo "✓ Copied bass.dll"
                else
                  echo "✗ Failed to copy bass.dll"
                  exit 1
                fi
              else
                echo "⚠ bass.dll not found in libs/ directory - BASS audio will not work"
              fi

              echo "🎵 Shortwave Radio deployed successfully!"
              echo "Files in XP directory:"
              ls -la "$XP_DIR"/*.{exe,dll} 2>/dev/null || echo "No files found"
            '';

            setup-dev = pkgs.writeShellScriptBin "setup-dev" ''
                            echo "Setting up development environment for Shortwave Radio..."

                            # Get dynamic paths from nix packages
                            GCC_BASE="${pkgs.pkgsCross.mingw32.buildPackages.gcc}/i686-w64-mingw32"
                            SYS_INCLUDE="$GCC_BASE/sys-include"
                            MINGW_MAIN_INCLUDE="${pkgs.pkgsCross.mingw32.windows.mingw_w64}/include"
                            CPP_INCLUDE="${pkgs.lib.getDev pkgs.pkgsCross.mingw32.buildPackages.gcc}/include/c++/10.3.0"
                            CPP_TARGET_INCLUDE="$CPP_INCLUDE/i686-w64-mingw32"

                            # Create .clangd config with correct paths
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
                  - -isystem
                  - $SYS_INCLUDE
                  - -isystem
                  - $MINGW_MAIN_INCLUDE
                  - -isystem
                  - $CPP_INCLUDE
                  - -isystem
                  - $CPP_TARGET_INCLUDE
                Remove:
                  - -I*/gcc/*/include
              EOF

                            # Create compile_commands.json
                            cat > compile_commands.json << EOF
              [
                {
                  "directory": "$(pwd)",
                  "command": "i686-w64-mingw32-g++ -DWINVER=0x0501 -D_WIN32_WINNT=0x0501 -DWIN32_LEAN_AND_MEAN -D_WIN32 -DWIN32 -std=c++17 -isystem \"$SYS_INCLUDE\" -isystem \"$MINGW_MAIN_INCLUDE\" -isystem \"$CPP_INCLUDE\" -isystem \"$CPP_TARGET_INCLUDE\" -c main.cpp",
                  "file": "main.cpp"
                }
              ]
              EOF

                            echo "Generated .clangd config and compile_commands.json with include paths"
                            echo "Development environment ready for Shortwave Radio development"
                            echo "Include paths:"
                            echo "  C standard library: $SYS_INCLUDE"
                            echo "  MinGW headers: $MINGW_MAIN_INCLUDE"
                            echo "  C++ headers: $CPP_INCLUDE"
            '';

            default = self'.packages.shortwave;
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
                                echo "Shortwave Radio development environment loaded"
                                echo "Available commands:"
                                echo "  nix build     - Build the Shortwave Radio application"
                                echo "  deploy-to-xp  - Deploy to XP VM folder"
                                echo ""
                                echo "Setting up development environment..."

                                # Get dynamic paths from nix packages
                                GCC_BASE="${pkgs.pkgsCross.mingw32.buildPackages.gcc}/i686-w64-mingw32"
                                SYS_INCLUDE="$GCC_BASE/sys-include"
                                MINGW_MAIN_INCLUDE="${pkgs.pkgsCross.mingw32.windows.mingw_w64}/include"
                                CPP_INCLUDE="${pkgs.lib.getDev pkgs.pkgsCross.mingw32.buildPackages.gcc}/include/c++/10.3.0"
                                CPP_TARGET_INCLUDE="$CPP_INCLUDE/i686-w64-mingw32"

                                # Auto-generate .clangd config with correct paths
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
                    - -isystem
                    - $SYS_INCLUDE
                    - -isystem
                    - $MINGW_MAIN_INCLUDE
                    - -isystem
                    - $CPP_INCLUDE
                    - -isystem
                    - $CPP_TARGET_INCLUDE
                  Remove:
                    - -I*/gcc/*/include
                EOF

                                cat > compile_commands.json << EOF
                [
                  {
                    "directory": "$(pwd)",
                    "command": "i686-w64-mingw32-g++ -DWINVER=0x0501 -D_WIN32_WINNT=0x0501 -DWIN32_LEAN_AND_MEAN -D_WIN32 -DWIN32 -std=c++17 -isystem \"$SYS_INCLUDE\" -isystem \"$MINGW_MAIN_INCLUDE\" -isystem \"$CPP_INCLUDE\" -isystem \"$CPP_TARGET_INCLUDE\" -c main.cpp",
                    "file": "main.cpp"
                  }
                ]
                EOF

                                echo "✓ Generated .clangd config and compile_commands.json with include paths"
                                echo "✓ Development environment ready for Shortwave Radio development"
              '';
            };
          };
        };
    };
}
