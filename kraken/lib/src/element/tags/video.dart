/*
 * Copyright (C) 2019-present Alibaba Inc. All rights reserved.
 * Author: Kraken Team.
 */

import 'dart:async';

import 'package:flutter/rendering.dart';
import 'package:kraken/element.dart';
import 'package:kraken_video_player/kraken_video_player.dart';

const String VIDEO = 'VIDEO';

class VideoParentData extends ContainerBoxParentData<RenderBox> {}

List<VideoPlayerController> videoControllers = [];

// dispose all video player when Dart VM is going to shutdown
Future<void> shutDownVideoPlayer() async {
  for (int i = 0; i < videoControllers.length; i ++) {
    await videoControllers[i].dispose();
  }
  videoControllers.clear();
}

class VideoElement extends Element {
  VideoPlayerController controller;

  String _src;
  String get src => _src;
  set src(String value) {
    if (_src != value) {
      bool needDispose = _src != null;
      _src = value;

      if (needDispose) {
        controller.dispose().then((_) {
          videoControllers.remove(controller);
          _removeVideoBox();

          _createVideoBox();
        });
      } else {
        _createVideoBox();
      }
    }
  }

  static const String DEFAULT_WIDTH = '300px';
  static const String DEFAULT_HEIGHT = '150px';

  @override
  void setDefaultProps(Map<String, dynamic> props) {
    if (props['style'] == null) {
      props['style'] = Map<String, dynamic>();
    }

    if (props['style']['width'] == null) {
      props['style']['width'] = DEFAULT_WIDTH;
    }

    if (props['style']['height'] == null) {
      props['style']['height'] = DEFAULT_HEIGHT;
    }
  }

  VideoElement(this.nodeId, this.props, this.events)
      : super(
          nodeId: nodeId,
          defaultDisplay: 'block',
          allowChildren: false,
          tagName: VIDEO,
          properties: props,
          events: events,
        );

  int nodeId;
  Map<String, dynamic> props;
  List<String> events;

  Future<int> createVideoPlayer(String src) {
    Completer<int> completer = new Completer();

    controller = VideoPlayerController.network(src);
    _src = src;

    controller.setLooping(props.containsKey('loop'));
    controller.onCanPlay = onCanPlay;
    controller.onCanPlayThrough = onCanPlayThrough;
    controller.onPlay = onPlay;
    controller.onPause = onPause;
    controller.onSeeked = onSeeked;
    controller.onSeeking = onSeeking;
    controller.onEnded = onEnded;
    controller.onError = onError;
    controller.initialize().then((int textureId) {
      if (props.containsKey('muted')) {
        controller.setMuted(true);
      }

      completer.complete(textureId);
    });

    videoControllers.add(controller);

    return completer.future;
  }

  void addVideoBox(int textureId) {
    RegExp exp = RegExp(r"^(http|https)://");

    if (props['src'] == null) {
      TextureBox box = TextureBox(textureId: 0);
      addChild(box);
      return;
    }

    if (!exp.hasMatch(props['src'])) {
      throw Exception('video url\'s prefix should be http:// or https://');
    }

    TextureBox box = TextureBox(textureId: textureId);

    addChild(box);

    if (props.containsKey('autoplay')) {
      controller.play();
    }
  }

  void _createVideoBox() {
    createVideoPlayer(_src).then((textureId) {
      addVideoBox(textureId);
    });
  }

  void _removeVideoBox() {
    renderPadding.child = null;
  }

  Future<Map<String, dynamic>> getVideoDetail() async {
    final Completer<Map<String, dynamic>> detailCompleter = Completer<Map<String, dynamic>>();
    RendererBinding.instance.addPostFrameCallback((Duration timeout) {
      var value = controller.value;
      var duration = value.duration;

      if (renderPadding.child != null) {
        Size size = renderPadding.size;
        detailCompleter.complete({
          'videoWidth': size.width,
          'videoHeight': size.height,
          'src': _src,
          'duration': '${duration.inSeconds}',
          'volume': value.volume,
          'position': value.position.inSeconds,
          'paused': !value.isPlaying,
        });
      }
    });
    return detailCompleter.future;
  }

  onCanPlay() async {
    Event event = Event('canplay', EventInit());
    event.detail = await getVideoDetail();
    dispatchEvent(event);
  }

  onCanPlayThrough() async {
    Event event = Event('canplaythrough', EventInit());
    event.detail = await getVideoDetail();
    dispatchEvent(event);
  }

  onEnded() async {
    Event event = Event('ended', EventInit());
    event.detail = await getVideoDetail();
    dispatchEvent(event);
  }

  onError(int code, String error) {
    Event event = Event('error', EventInit());
    event.detail = {'code': code, 'message': error};
    dispatchEvent(event);
  }

  onPause() async {
    Event event = Event('pause', EventInit());
    event.detail = await getVideoDetail();
    dispatchEvent(event);
  }

  onPlay() async {
    Event event = Event('play', EventInit());
    event.detail = await getVideoDetail();
    dispatchEvent(event);
  }

  onSeeked() async {
    Event event = Event('seeked', EventInit());
    event.detail = await getVideoDetail();
    dispatchEvent(event);
  }

  onSeeking() async {
    Event event = Event('seeking', EventInit());
    event.detail = await getVideoDetail();
    dispatchEvent(event);
  }

  onVolumeChange() async {
    Event event = Event('volumechange', EventInit());
    event.detail = await getVideoDetail();
    dispatchEvent(event);
  }

  @override
  dynamic method(String name, List<dynamic> args) {
    if (controller == null) {
      return;
    }

    switch (name) {
      case 'play':
        controller.play();
        break;
      case 'pause':
        controller.pause();
        break;
    }
  }

  @override
  void setProperty(String key, value) {
    super.setProperty(key, value);
    if (key == 'src') {
      src = value.toString();
    }
  }
}