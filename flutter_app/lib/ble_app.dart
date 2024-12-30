import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';
import 'dart:async';
import 'package:flutter/services.dart';

class MyBleApp extends StatefulWidget {
  const MyBleApp({super.key});

  @override
  State<MyBleApp> createState() => _MyBleAppState();
}

class _MyBleAppState extends State<MyBleApp> {
  final FlutterReactiveBle _ble = FlutterReactiveBle();

  StreamSubscription<DiscoveredDevice>? _scanSubscription;
  StreamSubscription<ConnectionStateUpdate>? _connectionSubscription;
  StreamSubscription<List<int>>? _notifySubscription;

  final String targetDeviceName = 'kitahatsu';
  final Uuid serviceUuid = Uuid.parse('c39e46c6-88d8-48d2-9310-278848445900');
  final Uuid characteristicUuid =
      Uuid.parse('68d37433-bd72-4ee1-95e0-b9f1d1e27dce');

  String _logText = 'Press "Scan" to start.';
  bool _scanning = false;
  int _retryCount = 0;
  final int _maxRetry = 3;

  // 受信データの履歴
  final List<String> _receivedData = [];

  // 5秒無視用のフラグ
  bool _canReceive = true;

  // ★ 受信回数をカウントする変数
  int _receivedCount = 0;

  @override
  void dispose() {
    _scanSubscription?.cancel();
    _connectionSubscription?.cancel();
    _notifySubscription?.cancel();
    super.dispose();
  }

  /// (1) Scan ボタンを押したら呼び出すメソッド
  void _startScan() {
    _stopScan();
    setState(() {
      _logText = 'Scanning...';
      _scanning = true;
      _receivedData.clear();
      _retryCount = 0;
      _canReceive = true; // 受け取り可能にリセット
      _receivedCount = 0; // カウントもリセット（要件に応じて変えてOK）
    });

    _scanSubscription = _ble.scanForDevices(
        withServices: [], scanMode: ScanMode.lowLatency).listen((device) {
      if (device.name == targetDeviceName) {
        _stopScan();
        _connect(device);
      }
    }, onError: (error) {
      setState(() {
        _logText = 'Scan error: $error';
      });
    });
  }

  /// スキャン停止
  void _stopScan() {
    _scanSubscription?.cancel();
    _scanSubscription = null;
    setState(() {
      _scanning = false;
    });
  }

  /// (2) デバイスに接続する
  void _connect(DiscoveredDevice device) {
    setState(() {
      _logText = 'Connecting to ${device.name}...';
    });

    _connectionSubscription = _ble
        .connectToDevice(
      id: device.id,
      connectionTimeout: const Duration(seconds: 5),
    )
        .listen((connectionState) async {
      final state = connectionState.connectionState;
      setState(() {
        _logText = 'State: $state';
      });

      if (state == DeviceConnectionState.connected) {
        await Future.delayed(const Duration(seconds: 1));
        _discoverServicesAndSubscribe(device);
      } else if (state == DeviceConnectionState.disconnected) {
        _attemptRetry(() => _connect(device));
      }
    }, onError: (error) {
      setState(() {
        _logText = 'Connection error: $error';
      });
      _attemptRetry(() => _connect(device));
    });
  }

  /// (3) 接続後、サービスを見つけてキャラクタリスティックの Notify を購読する
  Future<void> _discoverServicesAndSubscribe(DiscoveredDevice device) async {
    try {
      setState(() {
        _logText = 'Discovering services...';
      });
      final services = await _ble.getDiscoveredServices(device.id);
      debugPrint('Discovered services: $services');

      setState(() {
        _logText = 'Subscribing to Notify...';
      });

      final characteristic = QualifiedCharacteristic(
        serviceId: serviceUuid,
        characteristicId: characteristicUuid,
        deviceId: device.id,
      );

      _notifySubscription =
          _ble.subscribeToCharacteristic(characteristic).listen((data) {
        // 5秒間は受信を無視する
        if (!_canReceive) {
          return;
        }

        // ★ ここに来たら受信OKなので、受信回数をインクリメント
        _receivedCount++;

        final dataStr = utf8.decode(data);
        setState(() {
          // リストに追加
          _receivedData.add('[$_receivedCount] $dataStr');
          // ログ更新（要件に応じて表示を変えてOK）
          _logText = 'Received #$_receivedCount: $dataStr';
        });

        HapticFeedback.vibrate();

        // 5秒間は無視するためフラグをOFFに
        _canReceive = false;
        Future.delayed(const Duration(seconds: 1), () {
          // 5秒後に再び受信を有効化
          _canReceive = true;
        });
      }, onError: (error) {
        setState(() {
          _logText = 'Notify error: $error';
        });
        _attemptRetry(() => _discoverServicesAndSubscribe(device));
      });

      setState(() {
        _logText = 'Subscribed to notify successfully.';
      });
    } catch (e) {
      setState(() {
        _logText = 'Discover/Subscribe error: $e';
      });
      _attemptRetry(() => _discoverServicesAndSubscribe(device));
    }
  }

  /// (4) リトライ処理
  void _attemptRetry(Function action) {
    if (_retryCount < _maxRetry) {
      _retryCount++;
      setState(() {
        _logText += '\nRetry $_retryCount / $_maxRetry...';
      });
      Future.delayed(const Duration(seconds: 1), () {
        action();
      });
    } else {
      setState(() {
        _logText += '\nMax retry reached. Give up.';
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        // ログ表示
        Padding(
          padding: const EdgeInsets.all(16.0),
          child: Text(
            _logText,
            textAlign: TextAlign.center,
          ),
        ),

        // 受信データ一覧
        Expanded(
          child: ListView.builder(
            itemCount: _receivedData.length,
            itemBuilder: (context, index) {
              return ListTile(
                title: Text(_receivedData[index]),
              );
            },
          ),
        ),

        ElevatedButton(
          onPressed: _scanning
              ? null
              : () {
                  _startScan();
                },
          child: const Text('Scan'),
        ),
        const SizedBox(height: 16),
      ],
    );
  }
}
