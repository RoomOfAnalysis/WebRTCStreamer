#

## WebRTCStreamer

test webrtc streamer based on libdatachannel

most code from [libdatachannel streamer example](https://github.com/paullouisageneau/libdatachannel/tree/master/examples/streamer)

```sh
# dev dependency
vcpkg install libdatachannel[core,srtp,ws]:x64-windows --recurse
```

## Test

```sh
# signaling server
cd ./signaling_server
pip install -r requirements.txt
python ./signaling_server.py

# streamer
./streamer.exe xxx/h264/ xxx/opus/

# browser client
cd ./browser
python -m http.server --bind 127.0.0.1 8080
```
