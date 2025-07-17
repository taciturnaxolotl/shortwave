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
            if cp ${self'.packages.hello-world-app}/bin/HelloWorld.exe "$XP_DIR/$TEMP_NAME"; then
              echo "✓ Copied new version as $TEMP_NAME"
              
              # If the original exists, try to rename it
              if [ -f "$XP_DIR/$FINAL_NAME" ]; then
                if mv "$XP_DIR/$FINAL_NAME" "$XP_DIR/$OLD_NAME" 2>/dev/null; then
                  echo "✓ Backed up old version as $OLD_NAME"
                else
                  echo "⚠ Warning: Could not backup old version (file may be in use)"
                  echo "  Close the application on XP and try again, or manually rename files"
                  echo "  New version is available as: $TEMP_NAME"
                  exit 1
                fi
              fi
              
              # Move temp to final name
              if mv "$XP_DIR/$TEMP_NAME" "$XP_DIR/$FINAL_NAME"; then
                echo "✓ Deployed HelloWorld.exe successfully"
                
                # Clean up old backup if it exists
                if [ -f "$XP_DIR/$OLD_NAME" ]; then
                  rm -f "$XP_DIR/$OLD_NAME" 2>/dev/null || echo "  (Old backup file remains)"
                fi
              else
                echo "✗ Failed to finalize deployment"
                exit 1
              fi
            else
              echo "✗ Failed to copy new version"
              exit 1
            fi
            
            echo ""
            echo "Deployment complete! 🎉"
            echo "You can now run the updated application on XP"
          '';
          
          setup-dev = pkgs.writeShellScriptBin "setup-dev" ''
            echo "Setting up development environment for Zed..."
            
            # Get the proper MinGW headers - use the known path from our build
            GCC_BASE="/nix/store/l2gk3vvpdf33jf3gnfljyyx3dgwks8zp-i686-w64-mingw32-stage-final-gcc-debug-10.3.0/i686-w64-mingw32"
            SYS_INCLUDE="$GCC_BASE/sys-include"
            MINGW_MAIN_INCLUDE="/nix/store/hhbkp872dkayzd2qxfhkdc4rgn393g52-mingw-w64-i686-w64-mingw32-9.0.0-dev/include"
            MCFGTHREAD_INCLUDE="/nix/store/21c6w351iwpblnfz2m9v3ssvxcmqsz7h-mcfgthreads-i686-w64-mingw32-git-dev/include"
            CPP_INCLUDE="$GCC_BASE/include/c++/10.3.0"
            CPP_TARGET_INCLUDE="$CPP_INCLUDE/i686-w64-mingw32"
            
            # Verify paths exist
            if [ ! -f "$SYS_INCLUDE/stdlib.h" ]; then
              echo "Error: Could not find C standard library at $SYS_INCLUDE"
              exit 1
            fi
            
            if [ ! -f "$MINGW_MAIN_INCLUDE/windows.h" ]; then
              echo "Error: Could not find MinGW headers at $MINGW_MAIN_INCLUDE"
              exit 1
            fi
            
            if [ ! -f "$CPP_INCLUDE/vector" ]; then
              echo "Error: Could not find C++ standard library at $CPP_INCLUDE"
              exit 1
            fi
            
            if [ ! -f "$MCFGTHREAD_INCLUDE/mcfgthread/gthread.h" ]; then
              echo "Error: Could not find mcfgthread headers at $MCFGTHREAD_INCLUDE"
              exit 1
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
                - $SYS_INCLUDE
                - -isystem
                - $MINGW_MAIN_INCLUDE
                - -isystem
                - $MCFGTHREAD_INCLUDE
                - -isystem
                - $CPP_INCLUDE
                - -isystem
                - $CPP_TARGET_INCLUDE
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
            echo "Using C standard library: $SYS_INCLUDE"
            echo "Using MinGW headers: $MINGW_MAIN_INCLUDE"
            echo "Using mcfgthread headers: $MCFGTHREAD_INCLUDE"
            echo "Using C++ headers: $CPP_INCLUDE"
            echo ""
            echo "This includes complete C/C++ standard libraries and Win32 APIs"
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