using System;
using Gtk;

namespace IOCardTesterGTK
{
	class MainClass
	{
		public static void Main(string[] args)
		{
			Application.Init();
			new MainWindow().Show();
			Application.Run();
		}
	}
}
