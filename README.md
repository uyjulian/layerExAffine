# layerExAffine

This Kirikiri (2/Z) plugin provides a modular replacement for `Layer.affineCopy` and `Layer.operateAffine`.

## Building

After cloning submodules and placing `ncbind` and `tp_stub` in the parent directory, a simple `make` will generate `layerExAffine.dll`.

## How to use

After `Plugins.link("layerExAffine.dll");` is used, the additional functions will be exposed under the `Layer` class.

## License

This project is licensed under the MIT license. Please read the `LICENSE` file for more information.  
