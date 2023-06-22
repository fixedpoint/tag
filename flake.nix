{
  description = "A flake for building tag";

  inputs.nixpkgs.url = github:NixOS/nixpkgs/nixos-23.05;

  outputs = { self, nixpkgs }:
    let
      derivation_tag = system:
        with import nixpkgs { inherit system; };
        stdenv.mkDerivation {
          pname = "tag";
          version = "0.9.0";
          src = self;
          doCheck = true;
        };
    in {
      packages.aarch64-darwin.default = derivation_tag "aarch64-darwin";
      packages.aarch64-linux.default  = derivation_tag "aarch64-linux";
      packages.i686-linux.default     = derivation_tag "i686-linux";
      packages.x86_64-darwin.default  = derivation_tag "x86_64-darwin";
      packages.x86_64-linux.default   = derivation_tag "x86_64-linux";
    };
}
