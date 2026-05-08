using System.IO;

namespace LoogalWindow;

static class Program
{
    [STAThread]
    static void Main()
    {
        Directory.SetCurrentDirectory(AppContext.BaseDirectory);

        Directory.CreateDirectory(Path.Combine(AppContext.BaseDirectory, "data"));
        Directory.CreateDirectory(Path.Combine(AppContext.BaseDirectory, "data", "logs"));
        Directory.CreateDirectory(Path.Combine(AppContext.BaseDirectory, "manifests"));

        ApplicationConfiguration.Initialize();
        Application.Run(new Form1());
    }
}
