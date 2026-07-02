mergeInto(LibraryManager.library, {
  soldank_webrtc_connect: function (serverIpPtr, serverPort) {
    const serverIp = UTF8ToString(serverIpPtr);
    const state = Module.soldankWebRtc = {
      incoming: [],
      reliableQueue: [],
      unreliableQueue: [],
      reliable: null,
      unreliable: null,
      peerConnection: null,
    };

    const flush = function (channel, queue) {
      if (!channel || channel.readyState !== "open") {
        return;
      }
      while (queue.length > 0) {
        channel.send(queue.shift());
      }
    };

    const onMessage = function (event) {
      const pushBytes = function (data) {
        state.incoming.push(new Uint8Array(data));
      };
      if (event.data instanceof ArrayBuffer) {
        pushBytes(event.data);
      } else if (event.data && event.data.arrayBuffer) {
        event.data.arrayBuffer().then(pushBytes);
      }
    };

    (async function () {
      const pc = new RTCPeerConnection();
      state.peerConnection = pc;

      const signalingBaseUrl = "http://" + serverIp + ":" + serverPort;

      pc.onicecandidate = async function (event) {
        if (!event.candidate || !state.sessionId) {
          return;
        }
        await fetch(signalingBaseUrl + "/webrtc/candidate", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            session_id: state.sessionId,
            mid: event.candidate.sdpMid || "",
            candidate: event.candidate.candidate,
          }),
        });
      };

      state.unreliable = pc.createDataChannel("unreliable", {
        ordered: false,
        maxRetransmits: 0,
      });
      state.reliable = pc.createDataChannel("reliable", {
        ordered: true,
      });

      state.unreliable.binaryType = "arraybuffer";
      state.reliable.binaryType = "arraybuffer";
      state.unreliable.onmessage = onMessage;
      state.reliable.onmessage = onMessage;
      state.unreliable.onopen = function () { flush(state.unreliable, state.unreliableQueue); };
      state.reliable.onopen = function () { flush(state.reliable, state.reliableQueue); };

      const offer = await pc.createOffer();
      await pc.setLocalDescription(offer);

      const response = await fetch(signalingBaseUrl + "/webrtc/offer", {
        method: "POST",
        headers: { "Content-Type": "text/plain" },
        body: offer.sdp,
      });
      const answer = await response.json();
      state.sessionId = answer.session_id;
      await pc.setRemoteDescription({ type: answer.type, sdp: answer.sdp });

      for (const candidate of answer.candidates || []) {
        await pc.addIceCandidate({
          sdpMid: candidate.mid || null,
          candidate: candidate.candidate,
        });
      }
    })().catch(function (error) {
      console.error("WebRTC connection failed", error);
    });
  },

  soldank_webrtc_close: function () {
    const state = Module.soldankWebRtc;
    if (state && state.peerConnection) {
      state.peerConnection.close();
    }
  },

  soldank_webrtc_poll_message: function (bufferPtr, bufferSize) {
    const state = Module.soldankWebRtc;
    if (!state || state.incoming.length === 0) {
      return 0;
    }

    const message = state.incoming.shift();
    const bytesToCopy = Math.min(message.length, bufferSize);
    HEAPU8.set(message.subarray(0, bytesToCopy), bufferPtr);
    return bytesToCopy;
  },

  soldank_webrtc_send_unreliable: function (dataPtr, dataSize) {
    const state = Module.soldankWebRtc;
    if (!state) {
      return;
    }
    const data = HEAPU8.slice(dataPtr, dataPtr + dataSize);
    if (state.unreliable && state.unreliable.readyState === "open") {
      state.unreliable.send(data);
    } else {
      state.unreliableQueue.push(data);
    }
  },

  soldank_webrtc_send_reliable: function (dataPtr, dataSize) {
    const state = Module.soldankWebRtc;
    if (!state) {
      return;
    }
    const data = HEAPU8.slice(dataPtr, dataPtr + dataSize);
    if (state.reliable && state.reliable.readyState === "open") {
      state.reliable.send(data);
    } else {
      state.reliableQueue.push(data);
    }
  },
});
