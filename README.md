# Shortwave

an app for [rewind](https://rewind.hackclub.com/#)

![a screenshot of windows xp with a text editor open saying "this will be a very cool app soon; just you wait"](https://raw.githubusercontent.com/taciturnaxolotl/shortwave/main/.github/images/ss.webp)

## Install

You can download a pre-built binary from the releases or build it yourself using the nix flake.

## Development

this project uses nix for cross-compilation to windows xp. the key was using an older nixpkgs (22.05) since newer mingw toolchains use windows apis that don't exist in xp.

### Quick Start

```bash
# enter the dev environment (or use direnv)
nix develop

# build the app
nix build

# deploy to my xp vm folder
deploy-to-xp
```

### Editor Setup (Zed)

to get proper intellisense for win32 apis in zed:

```bash
# generate .clangd config with proper mingw headers
setup-dev

# restart zed
```

this creates a `.clangd` file that points to the actual mingw-w64 headers and avoids gcc intrinsics that cause clang issues.

### Build Details

- uses mingw-w64 cross-compiler targeting i686-w64-mingw32
- statically links runtime to avoid dll dependencies  
- targets windows xp (winver=0x0501) for maximum compatibility
- older nixpkgs (22.05) prevents "procedure entry point" errors on xp

~

written in cpp. If you have any suggestions or issues feel free to open an issue on my [tangled](https://tangled.sh/@dunkirk.sh/shortwave) knot

<p align="center">
	<img src="https://raw.githubusercontent.com/taciturnaxolotl/carriage/master/.github/images/line-break.svg" />
</p>

<p align="center">
	<i><code>&copy 2025-present <a href="https://github.com/taciturnaxolotl">Kieran Klukas</a></code></i>
</p>

<p align="center">
	<a href="https://github.com/taciturnaxolotl/shortwave/blob/master/LICENSE.md"><img src="https://img.shields.io/static/v1.svg?style=for-the-badge&label=License&message=MIT&logoColor=d9e0ee&colorA=363a4f&colorB=b7bdf8"/></a>
</p>
