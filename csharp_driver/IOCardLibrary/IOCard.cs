﻿using System;
using System.Collections.Generic;

namespace Spark.Slot.IO
{
	public class IOCard
	{
		public static IOCard Card { get; }

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
			/**
			 * button state not yet available, either
			 * 1. not received any update, yet
			 * 2. it's "masked", not an available input.
			 */
			StateUnavailable,
		}

		public UInt32 GetCachedCoinCount(CoinTrack trackId)
		{
			if (mCoinCounts.ContainsKey(trackId))
				return mCoinCounts[trackId];
			return 0;
		}

		public ButtonState GetCachedButtonState(int buttonId)
		{
			if (mButtonStates.ContainsKey(buttonId))
				return mButtonStates[buttonId];
			return ButtonState.StateUnavailable;
		}

		#region "implementations"
		// disallow instantiation
		private IOCard() {

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

			CMD_ACK = 0x0F, // special command used for ACK
		}

		public enum Events
		{
			EVT_ACK = 0x0F,
			EVT_COIN_COUNTER_RESULT = 0x2D,
			EVT_KEY = 0x4B,
			EVT_WRITE_STORAGE_RESULT = 0x69,
			EVT_READ_STORAGE_RESULT = 0x78,
		}

		#endregion
	}
}
