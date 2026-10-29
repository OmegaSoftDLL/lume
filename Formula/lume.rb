class Lume < Formula
  desc "AI-first programming language and toolchain"
  homepage "https://github.com/OmegaSoftDLL/lume"
  version "0.2.0"
  license any_of: ["MIT", "Apache-2.0"]

  on_macos do
    on_arm do
      url "https://github.com/OmegaSoftDLL/lume/releases/download/v0.2.0/lume-v0.2.0-macos-aarch64.tar.gz"
      sha256 "8672f7442a15aa8193032ef329e50f8dd8652713ee6cf15c9e7e8ca0d665f2fc"
    end
    on_intel do
      url "https://github.com/OmegaSoftDLL/lume/releases/download/v0.2.0/lume-v0.2.0-macos-x86_64.tar.gz"
      sha256 "12e2dc21d28644282eb6898b76b1244edc5ac44199a19033237b216fa2a8de83"
    end
  end

  on_linux do
    on_intel do
      url "https://github.com/OmegaSoftDLL/lume/releases/download/v0.2.0/lume-v0.2.0-linux-x86_64.tar.gz"
      sha256 "c69c89d7562696b20081ce83f854724e9348805cd414d6b1a6975f808d1e4f9b"
    end
  end

  def install
    libexec.install Dir["*"]
    bin.write_exec_script libexec/"lume"
  end

  test do
    assert_match "lume 0.2.0", shell_output("#{bin}/lume --version")
    (testpath/"hello.src").write "print 42\n"
    assert_equal "42\n", shell_output("#{bin}/lume run #{testpath}/hello.src")
  end
end
