final: prev: {
  stb = prev.stb.overrideAttrs (oldAttrs: {
    installPhase = ''
      runHook preInstall
      mkdir -p $out/include/stb
      cp *.h $out/include/stb/
      cp *.c $out/include/stb/
      runHook postInstall
    '';
  });
}
