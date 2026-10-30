class Lume < Formula
  desc "AI-first programming language and toolchain"
  homepage "https://github.com/OmegaSoftDLL/lume"
  version "0.2.0"
  license any_of: ["MIT", "Apache-2.0"]

  on_macos do
    on_arm do
      url "https://github.com/OmegaSoftDLL/lume/releases/download/v0.2.0/lume-v0.2.0-macos-aarch64.tar.gz"
      sha256 "46ecd3df54897c3f8a36930a4c363ae030cbb12a64b071dd5bad742c9b84e6c6"
    end
    on_intel do
      url "https://github.com/OmegaSoftDLL/lume/releases/download/v0.2.0/lume-v0.2.0-macos-x86_64.tar.gz"
      sha256 "f36f4e2c019d66c279b58bc2a311128defcd2aea01604e8004a7dbaff1ede1e8"
    end
  end

  on_linux do
    on_intel do
      url "https://github.com/OmegaSoftDLL/lume/releases/download/v0.2.0/lume-v0.2.0-linux-x86_64.tar.gz"
      sha256 "cfef9ad1d48af5d32ab8d464200a62afbba0a9011ce0a7799faf8e7d57a93ef8"
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
