{
  description = "Win32 Hello World App with About dropdown";

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
                cmake -DCMAKE_BUILD_TYPE=Release \
                      -DCMAKE_SYSTEM_NAME=Windows \
                      -DCMAKE_C_COMPILER=$CC \
                      -DCMAKE_CXX_COMPILER=$CXX \
                      -DCMAKE_EXE_LINKER_FLAGS="$LDFLAGS" .
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

              # Handle Windows file locking by using a temporary name first
              TEMP_NAME="HelloWorld_new.exe"
              OLD_NAME="HelloWorld_old.exe"
              FINAL_NAME="HelloWorld.exe"

              echo "Deploying to $XP_DIR..."

              # Copy to temporary name first
              if cp ${self'.packages.shortwave}/bin/HelloWorld.exe "$XP_DIR/$TEMP_NAME"; then
                echo "Copied new version as $TEMP_NAME"

                # If the original exists, try to rename it
                if [ -f "$XP_DIR/$FINAL_NAME" ]; then
                  if mv "$XP_DIR/$FINAL_NAME" "$XP_DIR/$OLD_NAME" 2>/dev/null; then
                    echo "Backed up old version as $OLD_NAME"
                  else
                    echo "Warning: Could not backup old version (file may be in use)"
                    echo "Close the application on XP and try again, or manually rename files"
                    echo "New version is available as: $TEMP_NAME"
                    exit 1
                  fi
                fi

                # Move temp to final name
                if mv "$XP_DIR/$TEMP_NAME" "$XP_DIR/$FINAL_NAME"; then
                  echo "Deployed HelloWorld.exe successfully"

                  # Clean up old backup if it exists
                  if [ -f "$XP_DIR/$OLD_NAME" ]; then
                    rm -f "$XP_DIR/$OLD_NAME" 2>/dev/null || echo "(Old backup file remains)"
                  fi
                else
                  echo "Failed to finalize deployment"
                  exit 1
                fi
              else
                echo "Failed to copy new version"
                exit 1
              fi
            '';

            setup-dev = pkgs.writeShellScriptBin "setup-dev" ''
                            echo "Setting up development environment for Zed..."

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
                            echo "Development environment ready for Zed editor"
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
                                echo "Win32 development environment loaded"
                                echo "Available commands:"
                                echo "  nix build     - Build the application"
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
                                echo "✓ Development environment ready for Zed editor"
              '';
            };
          };
        };
    };
}
