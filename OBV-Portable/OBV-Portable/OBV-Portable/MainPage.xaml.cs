using OBV;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Serialization;
using Xamarin.Essentials;
using Xamarin.Forms;

namespace OBV_Portable
{
    public partial class MainPage : ContentPage
    {
        public MainPage()
        {
            InitializeComponent();
        }

        private async void BtnLoadFile_Clicked(object sender, EventArgs e)
        {
            var pickOptions = new PickOptions
            {
                PickerTitle = "Select a board file",
                FileTypes = null
            };
            // Open a file picker to select a file
            var file = await FilePicker.PickAsync(pickOptions);

            if (file != null && file.FileName.EndsWith(".brd"))
            {
                await DisplayAlert("Success", "The file " + file.FileName + " will be loaded.", "OK");
                LoadFile(file);
            }
            else
            {
                await DisplayAlert("Error", "Please select a valid .brd file", "OK");
            }
        }

        private async void LoadFile(FileResult file)
        {
            // Get the file stream
            var stream = await file.OpenReadAsync();

            // Load the file as a list of characters
            var brdFile = new BRDFile(stream);
        }
    }
}
