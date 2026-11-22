<?php
header('Content-Type: application/json; charset=utf-8');
$dir = __DIR__ . '/data';
$files = is_dir($dir) ? glob($dir . '/*.json') : [];
$total = count($files);
$latest = null;
$probe = 0;
$karma = 0;
$ble = 0;
$passwords = 0;
if ($total > 0) {
    usort($files, function($a, $b) { return filemtime($b) - filemtime($a); });
    $latest = date('c', filemtime($files[0]));
    $data = json_decode(file_get_contents($files[0]), true);
    if (is_array($data)) {
        $probe = count($data['probe'] ?? []);
        $karma = count($data['karma'] ?? []);
        $ble = count($data['ble'] ?? []);
        $passwords = count($data['passwords'] ?? []);
    }
}
echo json_encode([
    'ok' => true,
    'files' => $total,
    'latest' => $latest,
    'probe' => $probe,
    'karma' => $karma,
    'ble' => $ble,
    'passwords' => $passwords
], JSON_UNESCAPED_SLASHES);
