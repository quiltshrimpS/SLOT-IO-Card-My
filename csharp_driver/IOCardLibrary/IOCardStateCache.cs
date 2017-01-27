using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;

namespace Spark.Slot.IO
{
	public class IOCardStateCache
	{
		public enum KeyState
		{
			StateLow,
			StateHigh,
			StateUnknown,
			StateNotAKey
		};

		/// <summary>
		/// The IO card object
		/// </summary>
		/// <value>The card.</value>
		public IOCard Card
		{
			get { return mCard; }
			set
			{
				lock (this)
				{
					// detach from old card
					if (mCard != null)
						_detach();

					mCard = value;

					// attach to the new card
					if (mCard != null)
						_attach();
				}
			}
		}

		/// <summary>The capacity of the error queue</summary>
		/// <value>
		/// The capacity of the error queue, defaults to 3000.
		/// If the errors queued up over this capacity, the oldest one is dropped.
		/// </value>
		public int ErrorQueueCapacity
		{
			get { return mErrorQueueCapacity; }
			set
			{
				lock (this)
				{
					mErrorQueueCapacity = value;

					IOCard.ErrorEventArgs error;
					if (!mErrors.IsEmpty)
					{
						Debug.WriteLine("Errors dropped due to capacity change:");
						while (mErrors.Count > mErrorQueueCapacity)
						{
							if (mErrors.TryDequeue(out error))
								Debug.WriteLine("    {0}: error {1}", error.TimeStamp, error.ErrorCode);
						}
					}
				}
			}
		}
		const int DEFAULT_ERROR_QUEUE_CAPACITY = 3000;

		/// <summary>
		/// The number of items in the error queue
		/// </summary>
		/// <value>The number of items in the error queue.</value>
		public int ErrorQueueCount { get { return mErrors.Count; } }

		/// <summary>
		/// Gets a value indicating whether this <see cref="T:Spark.Slot.IO.IOCardStateCache"/> is error queue empty.
		/// </summary>
		/// <value><c>true</c> if is error queue empty; otherwise, <c>false</c>.</value>
		public bool IsErrorQueueEmpty { get { return mErrors.IsEmpty; } }

		/// <summary>
		/// Gets the value indicating whether this <see cref="T:Spark.Slot.IO.IOCardStateCache"/> is changed.
		/// </summary>
		/// <value><c>true</c> if is changed; otherwise, <c>false</c>.</value>
		public bool IsChanged { get; internal set; }

		public string Manufacturer
		{
			get
			{
				lock (this)
					return mGetInfoResultEventArgs == null ? null : mGetInfoResultEventArgs.Manufacturer;
			}
		}

		public string Product
		{
			get
			{
				lock (this)
					return mGetInfoResultEventArgs == null ? null : mGetInfoResultEventArgs.Product;
			}
		}

		public string Version
		{
			get
			{
				lock (this)
					return mGetInfoResultEventArgs == null ? null : mGetInfoResultEventArgs.Version;
			}
		}

		public long ProtocolVersion
		{
			get
			{
				lock (this)
					return mGetInfoResultEventArgs == null ? -1L : mGetInfoResultEventArgs.ProtocolVersion;
			}
		}

		public IOCardStateCache(IOCard card = null, int error_capacity = DEFAULT_ERROR_QUEUE_CAPACITY)
		{
			ErrorQueueCapacity = error_capacity;
			Card = card;
		}

		/// <summary>
		/// Pops and return one error from the queue.
		/// </summary>
		/// <returns>The error, null if the queue is empty.</returns>
		public IOCard.ErrorEventArgs PopError()
		{
			IOCard.ErrorEventArgs error;
			if (mErrors.TryDequeue(out error))
				return error;
			return null;
		}

		/// <summary>
		/// Gets the coin counter.
		/// </summary>
		/// <returns>The coin counter.</returns>
		/// <param name="track">Track.</param>
		public uint GetCoinCounter(byte track)
		{
			lock (mCoinCounters)
			{
				uint coins;
				if (mCoinCounters.TryGetValue(track, out coins))
					return coins;
				return 0;
			}
		}

		/// <summary>
		/// Gets the key.
		/// </summary>
		/// <returns>The key.</returns>
		/// <param name="key">Key.</param>
		public KeyState GetKey(byte key)
		{
			lock (mKeyStates)
			{
				KeyState state;
				if (mKeyStates.TryGetValue(key, out state))
					return state;
				return KeyState.StateUnknown;
			}
		}

		/// <summary>
		/// Tell the cache that everything is processed.
		/// </summary>
		public void Processed()
		{
			IsChanged = false;
		}

		IOCard mCard;
		int mErrorQueueCapacity;

		IOCard.GetInfoResultEventArgs mGetInfoResultEventArgs;
		readonly Dictionary<byte, uint> mCoinCounters = new Dictionary<byte, uint>();
		readonly Dictionary<byte, KeyState> mKeyStates = new Dictionary<byte, KeyState>();
		readonly ConcurrentQueue<IOCard.ErrorEventArgs> mErrors = new ConcurrentQueue<IOCard.ErrorEventArgs>();

		void _detach()
		{
			mCard.OnConnected -= Card_OnConnected;
			mCard.OnDisconnected -= Card_OnDisconnected;

			mCard.OnKeyMasks -= Card_OnKeyMasks;
			mCard.OnKeys -= Card_OnKey;
			mCard.OnDebug -= Card_OnDebug;
			mCard.OnError -= Card_OnError;
			mCard.OnUnknown -= Card_OnUnknown;

			mCard.OnGetInfoResult -= Card_OnGetInfoResult;
			mCard.OnCoinCounterResult -= Card_OnCoinCounterResult;
		}

		void _attach()
		{
			mCard.OnConnected += Card_OnConnected;
			mCard.OnDisconnected += Card_OnDisconnected;

			mCard.OnKeyMasks += Card_OnKeyMasks;
			mCard.OnKeys += Card_OnKey;
			mCard.OnGetInfoResult += Card_OnGetInfoResult;
			mCard.OnCoinCounterResult += Card_OnCoinCounterResult;

			mCard.OnDebug += Card_OnDebug;
			mCard.OnError += Card_OnError;
			mCard.OnUnknown += Card_OnUnknown;
		}

		void Card_OnConnected(object sender, EventArgs e)
		{
			// query the card for initial states
			mCard.QueryGetInfo();
			mCard.QueryGetKeyMasks();
		}

		void Card_OnDisconnected(object sender, EventArgs e)
		{
			lock (this)
			{
				// clears out everything
				mGetInfoResultEventArgs = null;
				mCoinCounters.Clear();
				mKeyStates.Clear();
				if (!mErrors.IsEmpty)
				{
					Debug.WriteLine("{0} error(s) unprocessed before disconnect", mErrors.Count);
					IOCard.ErrorEventArgs error;
					while (mErrors.TryDequeue(out error))
						Debug.WriteLine("    {0}: error {1}", error.TimeStamp, error.ErrorCode);
				}
				IsChanged = true;
			}
		}

		void Card_OnKey(object sender, IOCard.KeysEventArgs e)
		{
			lock (mKeyStates)
			{
				for (int i = 0; i < e.Keys.Length; ++i)
				{
					for (int b = 0; b < 8; ++b)
					{
						var index = (byte)(i * 8 + b);
						if (!mKeyStates.ContainsKey(index) || mKeyStates[index] != KeyState.StateNotAKey)
							mKeyStates[index] = (e.Keys[i] & (1 << b)) != 0 ? KeyState.StateHigh : KeyState.StateLow;
					}
				}
			}
			lock (this)
				IsChanged = true;
		}

		void Card_OnKeyMasks(object sender, IOCard.KeyMasksEventArgs e)
		{
			lock (mKeyStates)
			{
				for (int i = 0; i < e.KeyMasks.Length; ++i)
				{
					for (int b = 0; b < 8; ++b)
					{
						if ((e.KeyMasks[i] & (1 << b)) == 0)
							mKeyStates[(byte)(i * 8 + b)] = KeyState.StateNotAKey;
					}
				}
			}
			lock (this)
				IsChanged = true;
		}

		void Card_OnDebug(object sender, IOCard.DebugEventArgs e)
		{
			Debug.WriteLine("{0}: {1}", e.DateTime, e.Message);
		}

		void Card_OnError(object sender, IOCard.ErrorEventArgs e)
		{
			IOCard.ErrorEventArgs error;
			while (mErrors.Count >= ErrorQueueCapacity)
			{
				if (mErrors.TryDequeue(out error))
					Debug.WriteLine("{0}: error queue full, error {1} received on {2} dropped.", DateTime.Now, error.ErrorCode, error.DateTime);
			}
			mErrors.Enqueue(e);
			lock (this)
				IsChanged = true;
		}

		void Card_OnUnknown(object sender, IOCard.UnknownEventArgs e)
		{
			Debug.WriteLine("unknow event received: {0}, raw = {1}", e.Command.CmdId, e.Command.RawString);
		}

		void Card_OnGetInfoResult(object sender, IOCard.GetInfoResultEventArgs e)
		{
			lock (this)
				mGetInfoResultEventArgs = e;
			lock (this)
				IsChanged = true;
		}

		void Card_OnCoinCounterResult(object sender, IOCard.CoinCounterResultEventArgs e)
		{
			lock (mCoinCounters)
				mCoinCounters[e.Track] = e.Coins;
			lock (this)
				IsChanged = true;
		}
	}
}
