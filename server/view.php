<?php
$dir = __DIR__ . '/data';
$files = [];
if (is_dir($dir)) {
    $files = glob($dir . '/*.json');
    usort($files, function($a, $b) { return filemtime($b) - filemtime($a); });
}

if (isset($_GET['clear'])) {
    foreach ($files as $f) @unlink($f);
    header('Location: view.php');
    exit;
}

$latest = null;
$latestFile = '';
if (!empty($files)) {
    $latestFile = basename($files[0]);
    $latest = json_decode(file_get_contents($files[0]), true);
}
function e($s){ return htmlspecialchars((string)$s, ENT_QUOTES, 'UTF-8'); }
function rssi_bar($rssi){
    $pct = max(0, min(100, ($rssi + 90) / 60 * 100));
    $col = $pct > 66 ? '#00e676' : ($pct > 33 ? '#ffca28' : '#ff5252');
    return '<div style="background:#333;border-radius:3px;height:6px;width:80px"><div style="background:'.$col.';height:6px;width:'.$pct.'%;border-radius:3px"></div></div>';
}
function trend_chart($files){
    $recent = array_slice(array_reverse($files), -15);
    $data = [];
    foreach ($recent as $f) {
        $j = json_decode(file_get_contents($f), true);
        $data[] = count($j['probe'] ?? []);
    }
    $max = max(1, max($data));
    $w = 300; $h = 80; $gap = 4;
    $bw = ($w - $gap * (count($data) - 1)) / max(1, count($data));
    $svg = '<svg width="100%" viewBox="0 0 '.$w.' '.$h.'" style="background:#1b1e24;border-radius:8px">';
    foreach ($data as $i => $v) {
        $bh = ($v / $max) * ($h - 20);
        $x = $i * ($bw + $gap);
        $y = $h - 20 - $bh;
        $svg .= '<rect x="'.$x.'" y="'.$y.'" width="'.$bw.'" height="'.$bh.'" fill="#00e676"></rect>';
    }
    $svg .= '</svg>';
    return $svg;
}
?>
<!DOCTYPE html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta http-equiv="refresh" content="5">
<title>NISKALA Cloud Data</title>
<style>
body{font-family:system-ui,sans-serif;background:#0f1115;color:#e6e6e6;margin:0;padding:16px}
h1{color:#00e676;font-size:20px;margin:0 0 4px}
h2{color:#4da3ff;font-size:16px;margin:18px 0 8px}
.sub{color:#9aa0a6;font-size:12px;margin:0 0 12px}
.card{background:#1b1e24;border-radius:10px;padding:12px;margin:10px 0}
table{width:100%;border-collapse:collapse;background:#1b1e24;border-radius:10px;overflow:hidden}
th,td{padding:8px 10px;border-bottom:1px solid #2a2f38;text-align:left;font-size:13px}
th{background:#22262e;color:#cfd6dd}
tr:last-child td{border-bottom:none}
.badge{display:inline-block;padding:2px 8px;border-radius:12px;font-size:11px;font-weight:bold}
.ok{background:#123a1e;color:#00e676}
.fail{background:#3a1212;color:#ff5252}
.unknown{background:#333;color:#ccc}
a{color:#4da3ff;text-decoration:none}
.btn-danger{display:inline-block;background:#b3261e;color:#fff;padding:8px 14px;border-radius:8px;font-weight:bold;font-size:13px}
ul{list-style:none;padding:0;margin:0}
li{padding:6px 0;border-bottom:1px solid #222;font-size:13px}
</style></head><body>
<h1>NISKALA Cloud Data</h1>
<p class="sub">Auto-refresh 5 detik &middot; Total file: <?php echo count($files); ?> &middot; <a href="view.php?clear=1" class="btn-danger" onclick="return confirm('Hapus semua data?')">Hapus Semua</a></p>

<?php if ($latest): ?>
<div class="card">
  <div><b>Sync terbaru:</b> <?php echo e($latestFile); ?></div>
  <div><b>Device:</b> <?php echo e($latest['device'] ?? '-'); ?> &middot; <b>MAC:</b> <?php echo e($latest['mac'] ?? '-'); ?> &middot; <b>Uptime:</b> <?php echo e($latest['uptime'] ?? '-'); ?>s</div>
</div>

<h2>Grafik Tren Probe</h2>
<?php echo trend_chart($files); ?>

<h2>Probe (<?php echo count($latest['probe'] ?? []); ?>)</h2>
<?php if (!empty($latest['probe'])): ?>
<table><tr><th>MAC</th><th>RSSI</th><th>Sinyal</th><th>SSID</th></tr>
<?php foreach($latest['probe'] as $p): ?>
<tr><td><?php echo e($p['mac'] ?? '-'); ?></td><td><?php echo e($p['rssi'] ?? '-'); ?></td><td><?php echo rssi_bar((int)($p['rssi'] ?? -90)); ?></td><td><?php echo e($p['ssid'] ?? '-'); ?></td></tr>
<?php endforeach; ?>
</table>
<?php else: ?><p class="sub">Tidak ada data probe.</p><?php endif; ?>

<h2>Karma (<?php echo count($latest['karma'] ?? []); ?>)</h2>
<?php if (!empty($latest['karma'])): ?>
<table><tr><th>SSID</th><th>Probe count</th><th>Auth count</th></tr>
<?php foreach($latest['karma'] as $k): ?>
<tr><td><?php echo e($k['ssid'] ?? '-'); ?></td><td><?php echo e($k['probe_count'] ?? '-'); ?></td><td><?php echo e($k['auth_count'] ?? '-'); ?></td></tr>
<?php endforeach; ?>
</table>
<?php else: ?><p class="sub">Tidak ada data karma.</p><?php endif; ?>

<h2>Passwords (<?php echo count($latest['passwords'] ?? []); ?>)</h2>
<?php if (!empty($latest['passwords'])): ?>
<table><tr><th>Email/Username</th><th>Password</th><th>Status</th></tr>
<?php foreach($latest['passwords'] as $pw): $v = $pw['verified'] ?? ''; ?>
<tr><td><?php echo e($pw['email'] ?? '-'); ?></td><td><?php echo e($pw['password'] ?? '-'); ?></td>
<td><?php if($v === '1') echo '<span class="badge ok">OK</span>'; elseif($v === '0') echo '<span class="badge fail">XX</span>'; else echo '<span class="badge unknown">?</span>'; ?></td></tr>
<?php endforeach; ?>
</table>
<?php else: ?><p class="sub">Tidak ada data password.</p><?php endif; ?>
<?php else: ?>
<p>Tidak ada data tersimpan.</p>
<?php endif; ?>

<h2>BLE Scan (<?php echo count($latest['ble'] ?? []); ?>)</h2>
<?php if (!empty($latest['ble'])): ?>
<table><tr><th>MAC</th><th>RSSI</th><th>Nama</th></tr>
<?php foreach($latest['ble'] as $b): ?>
<tr><td><?php echo e($b['mac'] ?? '-'); ?></td><td><?php echo e($b['rssi'] ?? '-'); ?></td><td><?php echo e($b['name'] ?? '-'); ?></td></tr>
<?php endforeach; ?>
</table>
<?php else: ?><p class="sub">Tidak ada data BLE.</p><?php endif; ?>

<h2>Riwayat File</h2>
<?php if (!empty($files)): ?>
<ul>
<?php foreach($files as $f): $n = basename($f); ?>
<li><?php echo e($n); ?> &middot; <?php echo date('Y-m-d H:i:s', filemtime($f)); ?></li>
<?php endforeach; ?>
</ul>
<?php else: ?><p class="sub">Kosong.</p><?php endif; ?>
</body></html>
