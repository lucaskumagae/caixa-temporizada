<?php
$remedios = [
    ["nome" => "Remédio 1", "horario" => "18:00"],
    ["nome" => "Remédio 2", "horario" => "18:00"],
    ["nome" => "Remédio 3", "horario" => "18:00"],
];
?>

<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <title>Meus Remédios</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <?php include 'nav.php'; ?>

    <main class="content">
        <header class="header">
            <h1>Meus remédios</h1>
            <button class="add-remedio">📅 Adicionar novo remédio</button>
        </header>

        <section class="lista-remedios">
            <?php foreach ($remedios as $remedio): ?>
                <div class="remedio">
                    <div class="icon-placeholder"></div>
                    <div class="info">
                        <strong><?= $remedio["nome"] ?></strong>
                        <p>Horários: <?= $remedio["horario"] ?></p>
                    </div>
                    <button class="editar">Editar</button>
                </div>
            <?php endforeach; ?>
        </section>
    </main>
</body>
</html>
