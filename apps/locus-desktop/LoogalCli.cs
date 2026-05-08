using System;
using System.Diagnostics;
using System.IO;
using System.Text.Json;
using System.Threading.Tasks;

public static class LoogalCli
{
    public static string FindEngine()
    {
        var candidates = new[]
        {
            Path.Combine(Directory.GetCurrentDirectory(), "..", "..", "loogal"),
            Path.Combine(Directory.GetCurrentDirectory(), "..", "..", "loogal.exe"),
            Path.Combine(AppContext.BaseDirectory, "loogal"),
            Path.Combine(AppContext.BaseDirectory, "loogal.exe")
        };

        foreach (var c in candidates)
        {
            var full = Path.GetFullPath(c);
            if (File.Exists(full))
                return full;
        }

        return OperatingSystem.IsWindows() ? "loogal.exe" : "loogal";
    }

    public static async Task<LoogalSearchResponse> SearchAsync(string image)
    {
        var repoRoot = Path.GetFullPath(Path.Combine(
            Directory.GetCurrentDirectory(),
            "..",
            ".."
        ));

        var psi = new ProcessStartInfo
        {
            FileName = FindEngine(),
            WorkingDirectory = repoRoot,
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };

        psi.ArgumentList.Add("search");
        psi.ArgumentList.Add(image);
        psi.ArgumentList.Add("--memory");
        psi.ArgumentList.Add("--json");
        psi.ArgumentList.Add("--limit");
        psi.ArgumentList.Add("200");
        psi.ArgumentList.Add("--min");
        psi.ArgumentList.Add("0");

        using var p = Process.Start(psi)!;

        var stdout = await p.StandardOutput.ReadToEndAsync();
        var stderr = await p.StandardError.ReadToEndAsync();

        await p.WaitForExitAsync();

        if (p.ExitCode != 0)
            throw new Exception(string.IsNullOrWhiteSpace(stderr) ? stdout : stderr);

        return JsonSerializer.Deserialize<LoogalSearchResponse>(
            stdout,
            new JsonSerializerOptions { PropertyNameCaseInsensitive = true })!;
    }

    public static void Reveal(string path)
    {
        if (OperatingSystem.IsWindows())
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = "explorer.exe",
                Arguments = $"/select,\"{path}\"",
                UseShellExecute = true
            });
        }
        else
        {
            var dir = Path.GetDirectoryName(path);
            if (!string.IsNullOrWhiteSpace(dir))
                Process.Start(new ProcessStartInfo
                {
                    FileName = "xdg-open",
                    ArgumentList = { dir },
                    UseShellExecute = false
                });
        }
    }
}
