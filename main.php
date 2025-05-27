<?php
include 'conexao.php';

$error_message = '';

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    if (isset($_POST['acao']) && $_POST['acao'] === 'adicionar') {
        $nome = $_POST['nome-remedio'] ?? '';
        $horario = $_POST['horario-remedio'] ?? '';

        if ($nome && $horario) {
            // verifica nomes repetidos
            $stmt = $conn->prepare("SELECT COUNT(*) FROM medicamento WHERE nome = ? AND ativo = TRUE");
            $stmt->bind_param("s", $nome);
            $stmt->execute();
            $stmt->bind_result($count);
            $stmt->fetch();
            $stmt->close();

            if ($count > 0) {
                $error_message = "Já existe um remédio com esse nome.";
            } else {
                $stmt = $conn->prepare("INSERT INTO medicamento (nome, ativo) VALUES (?, TRUE)");
                $stmt->bind_param("s", $nome);
                $stmt->execute();
                $id_medicamento = $stmt->insert_id;
                $stmt->close();

                $stmt = $conn->prepare("INSERT INTO horario_medicamento (id_medicamento, horario, repetir_diariamente) VALUES (?, ?, TRUE)");
                $stmt->bind_param("is", $id_medicamento, $horario);
                $stmt->execute();
                $stmt->close();

                header("Location: " . $_SERVER['PHP_SELF']);
                exit;
            }
        }
    } elseif (isset($_POST['acao']) && $_POST['acao'] === 'editar') {
        $id_medicamento = $_POST['id-medicamento'] ?? '';
        $nome = $_POST['nome-remedio-editar'] ?? '';
        $horario = $_POST['horario-remedio-editar'] ?? '';

        if ($id_medicamento && $nome && $horario) {
            $stmt = $conn->prepare("UPDATE medicamento SET nome = ? WHERE id_medicamento = ?");
            $stmt->bind_param("si", $nome, $id_medicamento);
            $stmt->execute();
            $stmt->close();

            $stmt = $conn->prepare("UPDATE horario_medicamento SET horario = ? WHERE id_medicamento = ? LIMIT 1");
            $stmt->bind_param("si", $horario, $id_medicamento);
            $stmt->execute();
            $stmt->close();

            header("Location: " . $_SERVER['PHP_SELF']);
            exit;
        }
    } elseif (isset($_POST['acao']) && $_POST['acao'] === 'excluir') {
        $id_medicamento = $_POST['id-medicamento'] ?? '';
        if ($id_medicamento) {
            // Delete from horario_medicamento 
            $stmt = $conn->prepare("DELETE FROM horario_medicamento WHERE id_medicamento = ?");
            $stmt->bind_param("i", $id_medicamento);
            $stmt->execute();
            $stmt->close();

            // Delete from medicamento
            $stmt = $conn->prepare("DELETE FROM medicamento WHERE id_medicamento = ?");
            $stmt->bind_param("i", $id_medicamento);
            $stmt->execute();
            $stmt->close();

            header("Location: " . $_SERVER['PHP_SELF']);
            exit;
        }
    }
}

$sql = "SELECT m.id_medicamento, m.nome, GROUP_CONCAT(DATE_FORMAT(h.horario, '%H:%i') SEPARATOR ', ') AS horarios
        FROM medicamento m
        LEFT JOIN horario_medicamento h ON m.id_medicamento = h.id_medicamento
        WHERE m.ativo = TRUE
        GROUP BY m.id_medicamento";
$result = $conn->query($sql);

$remedios = [];
if ($result) {
    while ($row = $result->fetch_assoc()) {
        $remedios[] = ['id' => $row['id_medicamento'], 'nome' => $row['nome'], 'horario' => $row['horarios']];
    }
}
?>

<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <title>Meus Remédios</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <?php if (!empty($error_message)): ?>
        <div style="position: fixed; top: 0; left: 0; right: 0; background-color: #f8d7da; color: #721c24; padding: 1em; border-bottom: 2px solid #f5c6cb; text-align: center; z-index: 1000;">
            <?= htmlspecialchars($error_message) ?>
        </div>
    <?php endif; ?>

    <?php include 'nav.php'; ?>

    <main class="content">
        <header class="header">
            <h1>Meus remédios</h1>
            <button class="add-remedio">📅 Adicionar novo remédio</button>
        </header>

        <section id="form-adicionar-remedio" style="display:none; padding: 1em; border: 1px solid #ccc; margin: 1em 0;">
            <h2>Adicionar novo remédio</h2>
            <form id="novo-remedio-form" method="POST" action="">
                <input type="hidden" name="acao" value="adicionar">
                <label for="nome-remedio">Nome do remédio:</label><br>
                <input type="text" id="nome-remedio" name="nome-remedio" required><br><br>
                <label for="horario-remedio">Horário:</label><br>
                <input type="time" id="horario-remedio" name="horario-remedio" required><br><br>
                <button type="submit">Adicionar</button>
                <button type="button" id="cancelar-adicionar">Cancelar</button>
            </form>
        </section>

        <section id="form-editar-remedio" style="display:none; padding: 1em; border: 1px solid #ccc; margin: 1em 0;">
            <h2>Editar remédio</h2>
            <form id="editar-remedio-form" method="POST" action="">
                <input type="hidden" name="acao" value="editar">
                <input type="hidden" id="id-medicamento" name="id-medicamento" value="">
                <label for="nome-remedio-editar">Nome do remédio:</label><br>
                <input type="text" id="nome-remedio-editar" name="nome-remedio-editar" required><br><br>
                <label for="horario-remedio-editar">Horário:</label><br>
                <input type="time" id="horario-remedio-editar" name="horario-remedio-editar" required><br><br>
                <button type="submit">Confirmar</button>
                <button type="button" id="cancelar-editar">Cancelar</button>
            </form>
        </section>

        <section class="lista-remedios">
            <?php foreach ($remedios as $remedio): ?>
                <div class="remedio" data-id="<?= $remedio['id'] ?>" data-nome="<?= htmlspecialchars($remedio['nome']) ?>" data-horario="<?= htmlspecialchars($remedio['horario']) ?>">
                    <div class="icon-placeholder"></div>
                    <div class="info">
                        <strong><?= htmlspecialchars($remedio["nome"]) ?></strong>
                        <p>Horários: <?= htmlspecialchars($remedio["horario"]) ?></p>
                    </div>
                <button class="editar">Editar</button>
                <button class="excluir">Excluir</button>
            </div>
        <?php endforeach; ?>
    </section>
    </main>

    <div id="confirm-excluir" style="display:none; position: fixed; top: 50%; left: 50%; transform: translate(-50%, -50%);
        background: white; border: 1px solid #ccc; padding: 1em; z-index: 2000; box-shadow: 0 0 10px rgba(0,0,0,0.5);">
        <p>Deseja realmente excluir este remédio?</p>
        <button id="confirmar-excluir">Confirmar</button>
        <button id="cancelar-excluir">Cancelar</button>
    </div>

    <script>
        document.querySelector('.add-remedio').addEventListener('click', function() {
            document.getElementById('form-adicionar-remedio').style.display = 'block';
        });

        document.getElementById('cancelar-adicionar').addEventListener('click', function() {
            document.getElementById('form-adicionar-remedio').style.display = 'none';
        });

        document.getElementById('cancelar-editar').addEventListener('click', function() {
            document.getElementById('form-editar-remedio').style.display = 'none';
        });

        document.querySelectorAll('.editar').forEach(function(button) {
            button.addEventListener('click', function() {
                var remedioDiv = this.closest('.remedio');
                var id = remedioDiv.getAttribute('data-id');
                var nome = remedioDiv.getAttribute('data-nome');
                var horario = remedioDiv.getAttribute('data-horario');

                document.getElementById('id-medicamento').value = id;
                document.getElementById('nome-remedio-editar').value = nome;
                // se tem varios horarios pega o primeiro
                var horarioPrimeiro = horario.split(',')[0].trim();
                document.getElementById('horario-remedio-editar').value = horarioPrimeiro;

                document.getElementById('form-editar-remedio').style.display = 'block';
            });
        });

        let remedioToDeleteId = null;
        document.querySelectorAll('.excluir').forEach(function(button) {
            button.addEventListener('click', function() {
                var remedioDiv = this.closest('.remedio');
                remedioToDeleteId = remedioDiv.getAttribute('data-id');
                document.getElementById('confirm-excluir').style.display = 'block';
            });
        });

        document.getElementById('cancelar-excluir').addEventListener('click', function() {
            remedioToDeleteId = null;
            document.getElementById('confirm-excluir').style.display = 'none';
        });

        document.getElementById('confirmar-excluir').addEventListener('click', function() {
            if (remedioToDeleteId) {
                var form = document.createElement('form');
                form.method = 'POST';
                form.action = '';

                var acaoInput = document.createElement('input');
                acaoInput.type = 'hidden';
                acaoInput.name = 'acao';
                acaoInput.value = 'excluir';
                form.appendChild(acaoInput);

                var idInput = document.createElement('input');
                idInput.type = 'hidden';
                idInput.name = 'id-medicamento';
                idInput.value = remedioToDeleteId;
                form.appendChild(idInput);

                document.body.appendChild(form);
                form.submit();
            }
        });
    </script>
</body>
</html>
