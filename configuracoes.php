<?php
if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $mysqli = new mysqli("localhost", "usuario", "senha", "nome_do_banco");

    if ($mysqli->connect_error) {
        die("Erro de conexão: " . $mysqli->connect_error);
    }

    $hora_inicio_np = $_POST['hora_inicio_np'];
    $hora_fim_np = $_POST['hora_fim_np'];
    $modo_nao_perturbe = isset($_POST['modo_nao_perturbe']) ? 1 : 0;

    $stmt = $mysqli->prepare("INSERT INTO configuracao (modo_nao_perturbe, hora_inicio_np, hora_fim_np) VALUES (?, ?, ?)");
    $stmt->bind_param("iss", $modo_nao_perturbe, $hora_inicio_np, $hora_fim_np);

    if ($stmt->execute()) {
        $mensagem = "Configuração salva com sucesso!";
    } else {
        $mensagem = "Erro ao salvar: " . $stmt->error;
    }

    $stmt->close();
    $mysqli->close();
}
?>

<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <title>Configurações</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
<?php include 'nav.php'; ?>
    <h1>Configuração do Modo Não Perturbe</h1>

    <?php if (isset($mensagem)) echo "<p><strong>$mensagem</strong></p>"; ?>

    <form method="post">
        <label for="hora_inicio_np">Horário de Início:</label>
        <input type="time" id="hora_inicio_np" name="hora_inicio_np" required><br><br>

        <label for="hora_fim_np">Horário de Fim:</label>
        <input type="time" id="hora_fim_np" name="hora_fim_np" required><br><br>

        <label for="modo_nao_perturbe">Ativar Modo Não Perturbe:</label>
        <input type="checkbox" id="modo_nao_perturbe" name="modo_nao_perturbe" value="1"><br><br>

        <button type="submit">Salvar Configuração</button>
    </form>
</body>
</html>
