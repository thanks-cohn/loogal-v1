using Avalonia.Controls;
using Avalonia.Media.Imaging;
using Avalonia.Input;
using Avalonia.Media;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace locus_desktop;

public partial class ResultsWindow : Window
{
    private readonly List<Border> _cards = new();
    private readonly List<LoogalResult> _results = new();
    private int _selectedIndex = 0;
    private bool _lightboxOpen = false;

    public ResultsWindow(LoogalSearchResponse response)
    {
        InitializeComponent();

        _results = response.Results.ToList();

        HeaderText.Text = $"LOCUS Results — showing {_results.Count} of {response.TotalHits} hits";

        SearchButton.Click += (_, _) =>
        {
            var win = new MainWindow();
            win.Show();
        };

        foreach (var r in _results)
            ResultsWrap.Children.Add(BuildCard(r, _cards.Count));

        KeyDown += OnKeyDown;

        Opened += (_, _) =>
        {
            Focus();
            if (_cards.Count > 0)
                SelectIndex(0);
        };
    }

    private Control BuildCard(LoogalResult r, int index)
    {
        var image = new Image
        {
            Width = 240,
            Height = 180,
            Stretch = Stretch.UniformToFill
        };

        try
        {
            if (File.Exists(r.Path))
                image.Source = new Bitmap(r.Path);
        }
        catch {}

        var title = new TextBlock
        {
            Text = $"{r.SimilarityPercent:0.##}% {r.Label}",
            FontWeight = FontWeight.Bold
        };

        var name = new TextBlock
        {
            Text = Path.GetFileName(r.Path),
            TextWrapping = TextWrapping.Wrap,
            MaxWidth = 220
        };

        var dir = new TextBlock
        {
            Text = Path.GetDirectoryName(r.Path) ?? "",
            TextWrapping = TextWrapping.Wrap,
            MaxWidth = 220,
            FontSize = 11,
            Opacity = 0.65,
            Cursor = new Cursor(StandardCursorType.Hand)
        };

        dir.PointerPressed += (_, _) => LoogalCli.Reveal(r.Path);

        var stack = new StackPanel
        {
            Width = 250,
            Margin = new Avalonia.Thickness(10),
            Children = { image, title, name, dir }
        };

        var border = new Border
        {
            CornerRadius = new Avalonia.CornerRadius(14),
            Padding = new Avalonia.Thickness(10),
            BorderThickness = new Avalonia.Thickness(2),
            BorderBrush = Brushes.Transparent,
            Background = Brushes.Transparent,
            Child = stack,
            Focusable = true
        };

        border.PointerEntered += (_, _) =>
        {
            if (index != _selectedIndex)
                border.BorderBrush = new SolidColorBrush(Color.FromArgb(90, 160, 160, 160));
        };

        border.PointerExited += (_, _) =>
        {
            if (index != _selectedIndex)
                border.BorderBrush = Brushes.Transparent;
        };

        border.PointerPressed += (_, e) =>
        {
            SelectIndex(index);

            if (e.ClickCount >= 2)
                OpenLightbox(index);
        };

        border.DoubleTapped += (_, _) =>
        {
            SelectIndex(index);
            OpenLightbox(index);
        };

        _cards.Add(border);
        return border;
    }

    private void OnKeyDown(object? sender, KeyEventArgs e)
    {
        if (_cards.Count == 0)
            return;

        if (_lightboxOpen)
        {
            if (e.Key == Key.Right)
            {
                SelectIndex(_selectedIndex + 1);
                UpdateLightbox();
                e.Handled = true;
            }
            else if (e.Key == Key.Left)
            {
                SelectIndex(_selectedIndex - 1);
                UpdateLightbox();
                e.Handled = true;
            }
            else if (e.Key == Key.Escape)
            {
                CloseLightbox();
                e.Handled = true;
            }

            return;
        }

        var columns = EstimateColumns();

        if (e.Key == Key.Right)
        {
            SelectIndex(_selectedIndex + 1);
            e.Handled = true;
        }
        else if (e.Key == Key.Left)
        {
            SelectIndex(_selectedIndex - 1);
            e.Handled = true;
        }
        else if (e.Key == Key.Down)
        {
            SelectIndex(_selectedIndex + columns);
            e.Handled = true;
        }
        else if (e.Key == Key.Up)
        {
            SelectIndex(_selectedIndex - columns);
            e.Handled = true;
        }
        else if (e.Key == Key.Enter || e.Key == Key.Space)
        {
            OpenLightbox(_selectedIndex);
            e.Handled = true;
        }
    }

    private int EstimateColumns()
    {
        var width = Bounds.Width;

        if (width <= 0)
            return 4;

        return Math.Max(1, (int)(width / 290));
    }

    private void SelectIndex(int index)
    {
        if (_cards.Count == 0)
            return;

        if (index < 0)
            index = 0;

        if (index >= _cards.Count)
            index = _cards.Count - 1;

        _selectedIndex = index;

        for (var i = 0; i < _cards.Count; i++)
        {
            if (i == _selectedIndex)
            {
                _cards[i].BorderBrush = new SolidColorBrush(Color.FromArgb(220, 175, 175, 175));
                _cards[i].Background = new SolidColorBrush(Color.FromArgb(28, 180, 180, 180));
            }
            else
            {
                _cards[i].BorderBrush = Brushes.Transparent;
                _cards[i].Background = Brushes.Transparent;
            }
        }

        _cards[_selectedIndex].Focus();
        _cards[_selectedIndex].BringIntoView();
    }

    private void OpenLightbox(int index)
    {
        SelectIndex(index);
        _lightboxOpen = true;
        Lightbox.IsVisible = true;
        UpdateLightbox();
        Focus();
    }

    private void CloseLightbox()
    {
        _lightboxOpen = false;
        Lightbox.IsVisible = false;
        Focus();
    }

    private void UpdateLightbox()
    {
        if (_selectedIndex < 0 || _selectedIndex >= _results.Count)
            return;

        var r = _results[_selectedIndex];

        try
        {
            LightboxImage.Source = File.Exists(r.Path) ? new Bitmap(r.Path) : null;
        }
        catch
        {
            LightboxImage.Source = null;
        }

        LightboxTitle.Text = $"{_selectedIndex + 1} / {_results.Count} · {r.SimilarityPercent:0.##}% {r.Label} · {Path.GetFileName(r.Path)}";
        LightboxPath.Text = r.Path;
    }
}
