<?php
/**
 * NISKALA Cloud Sync endpoint
 * Hanya menerima POST dengan header X-API-Key.
 */
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, X-API-Key');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['ok' => false, 'msg' => 'POST only']);
    exit;
}

define('API_KEY', 'niskala-secret-2026');
$key = isset($_SERVER['HTTP_X_API_KEY']) ? $_SERVER['HTTP_X_API_KEY'] : '';
if ($key !== API_KEY) {
    http_response_code(401);
    echo json_encode(['ok' => false, 'msg' => 'unauthorized']);
    exit;
}

$raw = file_get_contents('php://input');
if ($raw === false || trim($raw) === '') {
    echo json_encode(['ok' => false, 'msg' => 'no data']);
    exit;
}

$data = json_decode($raw, true);
if (!is_array($data)) {
    echo json_encode(['ok' => false, 'msg' => 'bad json']);
    exit;
}

$dir = __DIR__ . '/data';
if (!is_dir($dir)) {
    mkdir($dir, 0777, true);
}

$file = $dir . '/niskala_' . date('Ymd_His') . '_' . substr(md5($raw), 0, 6) . '.json';
file_put_contents($file, $raw . PHP_EOL, LOCK_EX);

echo json_encode(['ok' => true, 'file' => basename($file), 'bytes' => strlen($raw)]);
