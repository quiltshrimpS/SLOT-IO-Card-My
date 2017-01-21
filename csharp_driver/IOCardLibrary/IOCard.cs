using System;
using System.Collections.Generic;

using CommandMessenger;
using CommandMessenger.Transport;
using CommandMessenger.Transport.Serial;

namespace Spark.Slot.IO
{
	public class IOCard
	{
		private static readonly IOCard sCard = new IOCard();
		public static IOCard Card { get { return sCard; } }

		public enum CoinTrack
		{
			TrackInsert0 = 0x00,
			TrackInsert1,
			TrackInsert2,
			TrackEject0 = 0x80,
		}

		public enum ButtonState
		{
			StatePressed,
			StateReleased,
			/// <summary>
			///  button state not (yet) available.
			/// </summary>
			/// <value>
			/// 1. not received any update, yet
			/// 2. it's "masked", not an available input.
			/// </value>
			StateUnavailable,
		}

		public UInt32 GetCachedCoinCount(CoinTrack trackId)
		{
			if (mCoinCounts.ContainsKey(trackId))
				return mCoinCounts[trackId];
			return 0;
		}

		public ButtonState GetCachedButtonState(byte buttonId)
		{
			if (mButtonStates.ContainsKey(buttonId))
				return mButtonStates[buttonId];
			return ButtonState.StateUnavailable;
		}

		public bool Connect(string port)
		{
			if (mMessenger != null)
				return false;

			try
			{
				var transport = new SerialTransport()
				{
					CurrentSerialSettings = { PortName = port, BaudRate = 115200, DtrEnable = false, },
				};

				var messenger = new CmdMessenger(transport);
				messenger.Attach((int)Events.EVT_GET_INFO_RESULT, OnReceive_GetInfo);
				var status = messenger.Connect();
				if (status)
				{
					mMessenger = messenger;
					if (OnConnected != null)
						OnConnected(this, EventArgs.Empty);
				}
				return status;
			}
			catch (InvalidOperationException ex)
			{
				Console.WriteLine("IOCard.Connect(): connection failed, " + ex.Message);
			}
			return false;
		}

		public bool Disconnect()
		{
			if (IsConnected)
			{
				var status = mMessenger.Disconnect();
				if (status)
				{
					mMessenger = null;
					if (OnDisconnected != null)
						OnDisconnected(this, EventArgs.Empty);
				}
				return status;
			}
			return false;
		}

		public bool IsConnected { get { return mMessenger != null; } }

		public event EventHandler OnConnected;
		public event EventHandler OnDisconnected;

		#region "implementations"

		private CmdMessenger mMessenger = null;

		// disallow instantiation
		private IOCard()
		{
		}

		private Dictionary<CoinTrack, UInt32> mCoinCounts = new Dictionary<CoinTrack, UInt32>();
		private Dictionary<int, ButtonState> mButtonStates = new Dictionary<int, ButtonState>();

		public enum Commands
		{
			CMD_READ_STORAGE = 0x87,
			CMD_WRITE_STORAGE = 0x96,
			CMD_SET_OUTPUT = 0xB4,
			CMD_RESET_COIN_COINTER = 0xC3,
			CMD_GET_COIN_COUNTER = 0xD2,
			CMD_EJECT_COIN = 0xE1,
			CMD_GET_INFO = 0xF0,
		}

		public enum Events
		{
			EVT_GET_INFO_RESULT = 0x0F,
			EVT_COIN_COUNTER_RESULT = 0x2D,
			EVT_KEY = 0x4B,
			EVT_WRITE_STORAGE_RESULT = 0x69,
			EVT_READ_STORAGE_RESULT = 0x78,
		}

		#endregion
	}
}
